"""
Python Script for Apple Builds

Another reason for the custom build set-up for CUGL is iOS builds. There is a 
lot to set up in an iOS build beyond the source code. Note, however, that we do 
not separate iOS and macOS projects. We build one XCode project and put it in 
the Apple build folder.

Author: Walker M. White
Date:   10/29/25
"""
# NOTE: Avoid f strings as we need curly braces EVERYWHERE
import os, os.path
import shutil
import glob
from . import util


# The components of a pbxproj file
PBX_SEQUENCE = ['PBXHeader','PBXBuildFile','PBXContainerItemProxy','PBXFileReference',
                'PBXFrameworksBuildPhase','PBXGroup','PBXNativeTarget','PBXProject',
                'PBXReferenceProxy','PBXResourcesBuildPhase','PBXSourcesBuildPhase',
                'XCBuildConfiguration','XCConfigurationList','PBXFooter']

# These are unique to the template project
TARGET_ID = 'EB0F3C9527FB9DCB0037CC66'

# The subbuild folder for the project
MAKEDIR = 'apple'

# Supported header extensions
HEADER_EXT = ['.h', '.hh', '.hpp', '.hxx', '.hm', '.inl', '.inc']
# Supportes source extensions
SOURCE_EXT = ['.cpp', '.c', '.cc', '.cxx', '.m', '.mm','.asm', '.asmx','.swift']


# General XCode parsing
def parse_pbxproj(project):
    """
    Returns a dictionary composed of XCode objects
    
    An XCode object is a single XCode entity representing a feature like a file,
    a group, or a build setting. When we modify an XCode file, we do it by 
    adding and removing text at the object level. Parsing the XCode file into 
    its component objects makes it easier to modify.
    
    The resulting value is a dictionary that maps XCode PBX entries (see 
    PBX_SEQUENCE) to lists of objects (which are all strings).
    
    :param project: The Apple build directory
    :type project:  ``str``
    
    :return: The dictionary of XCode objects
    :rtype:  ``dict``
    """
    source = os.path.join(project,'project.pbxproj')
    contents = None
    with open(source) as file:
        index = 0
        state = PBX_SEQUENCE[index]
        block = []
        accum = None
        brace = 0
        contents = {state:block}
        between  = False
        
        for line in file:
            advance  = False
            if state != 'PBXFooter':
                advance = ('/* Begin '+PBX_SEQUENCE[index+1]+' section */') in line
            complete = ('/* End '+state+' section */') in line
            
            if advance or (complete and index == len(PBX_SEQUENCE)-2):
                # Time to advance state
                if accum:
                    block.append(accum)
                index += 1
                state = PBX_SEQUENCE[index]
                block = []
                accum = None
                brace = 0
                contents[state] = block
                between = False
            elif state in ['PBXHeader','PBXFooter']:
                accum = line if accum is None else accum+line
            elif complete:
                if accum and accum != '\n':
                    block.append(accum)
                    between = True
            elif not between:
                # Now it is time to parse objects
                brace += util.group_parity(line,'{}')
                if brace < 0:
                    print('ERROR: Braces mistmatch in pbxproj file')
                    return None
                else:
                    accum = line if accum is None else accum+line
                    if brace == 0:
                        if accum != '\n':
                            block.append(accum)
                        accum = None
        
        # Do not forget the footer
        block.append(accum)
    
    return contents


def write_pbxproj(pbxproj,project):
    """
    Recollates the XCode objects back into a PBX file in the build directory.
    
    :param pbxproj: The dictionary of XCode objects
    :type pbxproj:  ``dict``
    
    :param project: The Apple build directory
    :type project:  ``str``
    """
    path = os.path.join(project,'project.pbxproj')
    with open(path,'w') as file:
        for state in pbxproj:
            if not state in ['PBXHeader', 'PBXFooter']:
                file.write('/* Begin '+state+' section */\n')
            for item in pbxproj[state]:
                #file.write(item.replace('__DISPLAY_NAME__',name))
                file.write(item)
            if not state in ['PBXHeader', 'PBXFooter']:
                file.write('/* End '+state+' section */\n\n')

# Template specific functions
def determine_orientation(orientation):
    """
    Returns the Apple orientation corresponding the config setting
    
    :param orientation: The orientation setting
    :type orientation:  ``str``
    
    :return: the Apple orientation corresponding the config setting
    :rtype:  ``str``
    """
    if orientation == 'portrait':
        return 'UIInterfaceOrientationPortrait'
    elif orientation == 'landscape':
        return 'UIInterfaceOrientationLandscapeRight'
    elif orientation == 'portrait-flipped':
        return 'UIInterfaceOrientationPortraitUpsideDown'
    elif orientation == 'landscape-flipped':
        return 'UIInterfaceOrientationLandscapeLeft'
    elif orientation == 'portrait-either':
        return '"UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown"'
    elif orientation == 'landscape-either':
        return '"UIInterfaceOrientationLandscapeRight UIInterfaceOrientationLandscapeLeft"'
    elif orientation == 'multidirectional':
        return '"UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeRight"'
    elif orientation == 'omnidirectional':
        return '"UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeRight UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationPortraitUpsideDown"'

    return 'UIInterfaceOrientationLandscapeRight'


def build_groups(path,filetree,uuidsvc):
    """
    Creates the groups for the XCode project
    
    Groups are virtual directories in XCode. They must each have a unique UUID
    and be connected in a tree-like manner. This function returns a dictionary
    representing this tree.  The keys are the unique UUIDS and the values are
    three-element tuples representing each child.  These tuples contain the
    child UUID, the child path, and an annotation.  If the path is a leaf in
    the filetree, the annotation is the same as the filetree. Otherwise it is
    simply the tag 'group'.
    
    :param path: The path to the root of the filetree in POSIX form
    :type config:  ``str``
    
    :param filetree: The filetree of paths
    :type filetree:  ``dict``
    
    :param uuidsvc: The UUID generator
    :type uuidsvc:  ``UUIDService``
    
    :return: The dictionary of the groups together with the UUID of the root
    :rtype:  (``str``,``dict``)
    """
    uuid = uuidsvc.getAppleUUID('GROUP://'+path)
    uuid = uuidsvc.applyPrefix('CD',uuid)
    contents = []
    result = {uuid:('',contents)}
    for item in filetree:
        if type(filetree[item]) == dict: # This is a group
            subresult = build_groups(path+'/'+item,filetree[item],uuidsvc)
            contents.append((subresult[0],item,'group'))
            for key in subresult[1]:
                if key == subresult[0]:
                    result[key] = (item,subresult[1][key][1])
                else:
                    result[key] = subresult[1][key]
        else:
            newid = uuidsvc.getAppleUUID('FILE://'+path+'/'+item)
            newid = uuidsvc.applyPrefix('BA',newid)
            contents.append((newid,item,filetree[item]))
    
    return (uuid,result)


def place_project(config):
    """
    Places the XCode project in the build directory
    
    :param config: The project configuration settings
    :type config:  ``dict``
    
    :return: The project directory
    :rtype:  ``str``
    """
    entries = ['root','build','camel']
    util.check_config_keys(config,entries)
    
    # Create the build folder if necessary
    build = config['build']
    if not os.path.exists(build):
        os.mkdir(build)
    
    # Clear and create the temp folder
    build = util.remake_dir(build,MAKEDIR)
    
    # Copy the XCode project
    template = os.path.join(config['sdl3'],'templates','apple','app.xcodeproj')
    project  = os.path.join(build,config['camel']+'.xcodeproj')
    shutil.copytree(template, project, copy_function = shutil.copy)
    
    # Now copy the resources folder
    src = os.path.join(config['sdl3'],'templates','apple','Resources')
    dst  = os.path.join(build,'Resources')
    shutil.copytree(src, dst, copy_function = shutil.copy)
    
    return project


def place_entries(obj,entries,prefix='files'):
    """
    Returns a copy of XCode object with the given entries added
    
    An XCode object is one recognized by parse_pbxproj. Entries are typically 
    written between two parentheses, with a parent value such as 'files' or 
    'children'. This function adds these entries to the parentheses, properly 
    indented (so the entries should not be indented beforehand).
    
    :param obj: The XCode object to modify
    :type obj:  ``str``
    
    :param entries: The entries to add
    :type entries:  ``list`` of ``str``
    
    :param prefix: The parent annotation for the child list
    :type prefix:  ``str``
    
    :return: the object with the given entries added
    :rtype:  ``str``
    """
    indent = '\n\t\t\t\t'
    line = obj.find(prefix+' = (')
    if line == -1:
        return
    pos = obj.find('\n',line)
    children = indent+indent.join(entries)
    return obj[:pos]+children+obj[pos:]


def reassign_pbxproj(config,pbxproj):
    """
    Modifies pbxproj to reflect the current build location
    
    The template XCode project has several variables that begin and end with 
    double underscores. The vast  majority of these are directories that must 
    be updated to match the current location of the build directory. This 
    function updates all of those variables
    
    :param config: The project configuration settings
    :type config:  ``dict``
    
    :param pbxproj: The dictionary of XCode objects
    :type pbxproj:  ``dict``
    """
    # SDL directory (relative)
    sdl3dir = '../'+util.path_to_posix(config['build_to_sdl3'])
    rootdir = '../'+util.path_to_posix(config['build_to_root'])
    
    # Replace the project path
    section = pbxproj['PBXFileReference']
    index = -1
    for pos in range(len(section)):
        if 'sdl3app.xcodeproj' in section[pos]:
            entry = section[pos]
            pos0 = entry.find('path')
            pos1 = entry.find(';',pos0)
            section[pos] = entry[:pos0]+'path = '+sdl3dir+'/buildfiles/apple/sdl3app.xcodeproj'+entry[pos1:]
    
    # Asset and Source directory
    section = pbxproj['PBXGroup']
    assetdir  = rootdir+'/'+util.path_to_posix(config['assets'])
    sourcedir = rootdir
    
    if len(config['source_tree']) == 1:
        sourcedir += '/'+util.path_to_posix(list(config['source_tree'].keys())[0])
    
    for pos in range(len(section)):
        if '__ASSET_DIR__' in section[pos]:
            section[pos] = section[pos].replace('__ASSET_DIR__','"'+assetdir+'"')
        if '__SOURCE_DIR__' in section[pos]:
            section[pos] = section[pos].replace('__SOURCE_DIR__','"'+sourcedir+'"')
    
    # Expand the remaining build settings (includes, display name, target id)
    section = pbxproj['XCBuildConfiguration']
    groupdir = lambda x: '"../../'+util.path_to_posix(x)+'"'
    
    indent = '\t\t\t\t\t'
    entries = config['include_dict']
    if 'all' in entries and entries['all']:
        allincludes = config['include_dict']['all']
    else:
        allincludes = []
    if 'apple' in entries and entries['apple']:
        allincludes += config['include_dict']['all']
    else:
        allincludes += []
    if allincludes:
        allincludes = indent+(',\n'+indent).join(map(groupdir,allincludes))+',\n'
    else:
        allincludes = ''
    
    appid = config['appid']
    for pos in range(len(section)):
        if '__SDL_INCLUDE__' in section[pos]:
            section[pos] = section[pos].replace('__SDL_INCLUDE__','"'+sdl3dir+'/include"')
        if '__APPLE_INCLUDE__' in section[pos]:
            section[pos] = section[pos].replace(indent+'__APPLE_INCLUDE__,\n',allincludes)
        if '__APP_ID__' in section[pos]:
            section[pos] = section[pos].replace('__APP_ID__',appid)
    
    # Update the target
    for category in ['PBXProject','XCConfigurationList','PBXNativeTarget']:
        section = pbxproj[category]
        for pos in range(len(section)):
            if 'main-app' in section[pos]:
                section[pos] = section[pos].replace('main-app',config['short'].lower())
    
    # Finally, update the display name everywhere
    for category in pbxproj:
        section = pbxproj[category]
        for pos in range(len(section)):
            if '__DISPLAY_NAME__.app' in section[pos]:
                section[pos] = section[pos].replace('__DISPLAY_NAME__.app','"%s.app"' % config['name'])
            elif '__DISPLAY_NAME__' in section[pos]:
                section[pos] = section[pos].replace('__DISPLAY_NAME__','"%s"' %config['name'])


def assign_orientation(config,pbxproj):
    """
    Assigns the default orientation for a mobile build
    
    Both iPhone and iPad builds receive the same orientation by default.
    
    :param config: The project configuration settings
    :type config:  ``dict``
    
    :param pbxproj: The dictionary of XCode objects
    :type pbxproj:  ``dict``
    """
    orientation = determine_orientation(config['orientation'])
    portrait = 'Portrait' in orientation
    # XCBuildConfiguration
    section = pbxproj['XCBuildConfiguration']
    for pos in range(len(section)):
        data = section[pos].split('\n')
        change = False
        for ii in range(len(data)):
            if portrait and 'INFOPLIST_KEY_UILaunchStoryboardName' in data[ii]:
                data[ii] = '\t\t\t\t"INFOPLIST_KEY_UILaunchStoryboardName[sdk=iphone*]" = Portrait;'
                change = True
            if portrait and 'INFOPLIST_KEY_UIMainStoryboardFile' in data[ii]:
                data[ii] = '\t\t\t\t"INFOPLIST_KEY_UIMainStoryboardFile[sdk=iphone*]" = Portrait;'
                change = True
            if 'INFOPLIST_KEY_UISupportedInterfaceOrientations' in data[ii]:
                data[ii] = '\t\t\t\tINFOPLIST_KEY_UISupportedInterfaceOrientations = '+orientation+';'
                change = True
            if 'INFOPLIST_KEY_UISupportedInterfaceOrientations_iPad' in data[ii]:
                data[ii] = '\t\t\t\tINFOPLIST_KEY_UISupportedInterfaceOrientations_iPad = '+orientation+';'
                change = True
            if 'INFOPLIST_KEY_UISupportedInterfaceOrientations_iPhone' in data[ii]:
                data[ii] = '\t\t\t\tINFOPLIST_KEY_UISupportedInterfaceOrientations_iPhone = '+orientation+';'
                change = True
        if change:
            section[pos] = '\n'.join(data)


def populate_assets(config,pbxproj):
    """
    Adds the assets to the XCode project
    
    While assets are all collected in a single folder, we do not add that folder 
    by itself. Instead we add all of the top-level entries of that folder. Files 
    are added directly. Subfolders are added by reference.
    
    :param config: The project configuration settings
    :type config:  ``dict``
    
    :param pbxproj: The dictionary of XCode objects
    :type pbxproj:  ``dict``
    """
    uuidsvc = config['uuids']
    
    # Locate the resource entries
    the_rsc = None
    # Find the resources
    for obj in pbxproj['PBXNativeTarget']:
        pos1 = obj.find('/* Resources */')
        pos0 = obj.rfind('\n',0,pos1)
        if pos0 != -1 and pos1 != -1:
            if TARGET_ID in obj:
                the_rsc = obj[pos0:pos1].strip()
    
    # Put in assets
    children = []
    the_ref = []
    for asset in config['asset_list']:
        uuid = uuidsvc.getAppleUUID('ASSET://'+asset[0])
        uuid = uuidsvc.applyPrefix('AA',uuid)
        
        apath = util.path_to_posix(asset[0])
        children.append('%s /* %s */,' % (uuid,asset[0]))
        if asset[1] == 'file':
            entry = '\t\t%s /* %s */ = {isa = PBXFileReference; path = %s; sourceTree = "<group>"; };\n' % (uuid,apath,apath)
        else:
            entry = '\t\t%s /* %s */ = {isa = PBXFileReference; lastKnownFileType = folder; path = %s; sourceTree = "<group>"; };\n' % (uuid,apath,apath)
        pbxproj['PBXFileReference'].append(entry)
        
        newid = uuidsvc.getAppleUUID('CATALYST://'+uuid)
        newid = uuidsvc.applyPrefix('AB',newid)
        entry = '\t\t%s /* %s in Resources */ = {isa = PBXBuildFile; fileRef = %s /* %s */; };\n' % (newid, apath, uuid, apath)
        the_ref.append('%s /* %s in Resources */,' % (newid,asset[0]))
        pbxproj['PBXBuildFile'].append(entry)
        
    # Add them to the group
    groups = pbxproj['PBXGroup']
    for pos in range(len(groups)):
        if '/* Assets */ =' in groups[pos]:
            groups[pos] = place_entries(groups[pos],children,'children')
    
    # Add them to the resources
    groups = pbxproj['PBXResourcesBuildPhase']
    for pos in range(len(groups)):
        if the_rsc and the_rsc in groups[pos]:
            groups[pos] = place_entries(groups[pos],the_ref)


def populate_sources(config,pbxproj):
    """
    Adds the source code files to the XCode project
    
    This method builds a filetree so that are can organize subdirectories as 
    explicit groups in XCode. With MacCatalyst support, we no longer support
    separating source in to macOS and iOS files. That fine level of control
    should be done directly in XCode for now.
    
    :param config: The project configuration settings
    :type config:  ``dict``
    
    :param pbxproj: The dictionary of XCode objects
    :type pbxproj:  ``dict``
    """
    uuidsvc = config['uuids']
    filetree = config['source_tree']
    sourcedir = '../'+util.path_to_posix(config['build_to_root'])
    if len(config['source_tree']) == 1:
        key = list(config['source_tree'].keys())[0]
        sourcedir += '/'+util.path_to_posix(key)
        filetree = filetree[key]
    
    uuid, result = build_groups(sourcedir,filetree,uuidsvc)
    
    # Get the top level elements
    files = []
    tops  = result[uuid]
    entries = []
    for item in tops[1]:
        if item[2] != 'group':
            files.append(item)
        entries.append('%s /* %s */,' % (item[0],util.path_to_posix(item[1])))
    
    # Find the root
    groups = pbxproj['PBXGroup']
    for pos in range(len(groups)):
        if '/* Source */ =' in groups[pos]:
            groups[pos] = place_entries(groups[pos],entries,'children')
    
    # Append all groups that are not root
    for key in result:
        if key != uuid:
            text  = '\t\t%s /* %s */ = {\n' % (key,result[key][0])
            text += '\t\t\tisa = PBXGroup;\n\t\t\tchildren = (\n'
            for item in result[key][1]:
                if item[2] != 'group':
                    files.append(item)
                text += '\t\t\t\t%s /* %s */,\n' % (item[0],util.path_to_posix(item[1]))
            text += '\t\t\t);\n\t\t\tpath = %s;\n' % util.path_to_posix(result[key][0])
            text += '\t\t\tsourceTree = "<group>";\n\t\t};\n'
            groups.append(text)
    
    # Now populate the files
    the_ref = []
    for item in files:
        ipath = util.path_to_posix(item[1])
        entry = '\t\t%s /* %s */ = {isa = PBXFileReference; fileEncoding = 4; path = %s; sourceTree = "<group>"; };\n' % (item[0],ipath,ipath)
        pbxproj['PBXFileReference'].append(entry)
        
        if os.path.splitext(item[1])[1] in SOURCE_EXT:
            if item[2] in ['all','apple']:
                newid = uuidsvc.getAppleUUID('CATALYST://'+item[0])
                newid = uuidsvc.applyPrefix('BB',newid)
                entry = '\t\t%s /* %s in Sources */ = {isa = PBXBuildFile; fileRef = %s /* %s */; };\n' % (newid, ipath, item[0], ipath)
                the_ref.append('%s /* %s in Sources */,' % (newid,ipath))
                pbxproj['PBXBuildFile'].append(entry)
    
    # Locate the source entries
    the_src = None
    for obj in pbxproj['PBXNativeTarget']:
        pos1 = obj.find('/* Sources */')
        pos0 = obj.rfind('\n',0,pos1)
        if pos0 != -1 and pos1 != -1:
            if TARGET_ID in obj:
                the_src = obj[pos0:pos1].strip()
    
    # Add them to the builds
    groups = pbxproj['PBXSourcesBuildPhase']
    for pos in range(len(groups)):
        if the_src and the_src in groups[pos]:
            groups[pos] = place_entries(groups[pos],the_ref)


def update_schemes(config,project):
    """
    Updates the schemes to match the new application name.
    
    Note that schemes are separate files and NOT in the PBX file.
    
    :param config: The project configuration settings
    :type config:  ``dict``
    
    :param project: The Apple build directory
    :type project:  ``str``
    """
    targets = config['targets'] if type(config['targets']) == list else [config['targets']]
    
    management = os.path.join(project,'xcshareddata','xcschemes','xcschememanagement.plist')
    context = { 'main-app':config['short'].lower() }
    util.file_replace(management,context)
    
    name = config['short'].lower()
    src = os.path.join(project,'xcshareddata','xcschemes','app.xcscheme')
    dst = os.path.join(project,'xcshareddata','xcschemes',name+'.xcscheme')
    if 'apple' in targets:
        shutil.move(src,dst)
        context = {'__DISPLAY_NAME__':config['name'],'main-app':name,'container:app.xcodeproj':'container:'+config['camel']+'.xcodeproj'}
        util.file_replace(dst,context)
    else:
        os.remove(src)


def make(config):
    """
    Creates the XCode project
    
    This only creates the XCode project; it does not actually build the project. 
    To build the project, you must open it up in XCode.
    
    :param config: The project configuration settings
    :type config:  ``dict``
    """
    print()
    print('Configuring Apple build files')
    print('-- Copying XCode project')
    project = place_project(config)
    pbxproj = parse_pbxproj(project)
    print('-- Modifying project settings')
    reassign_pbxproj(config,pbxproj)
    assign_orientation(config,pbxproj)
    print('-- Populating project file')
    populate_assets(config,pbxproj)
    populate_sources(config,pbxproj)
    print('-- Retargeting builds')
    write_pbxproj(pbxproj,project)
    update_schemes(config,project)
    
    if 'icon' in config:
        print('-- Generating icons')
        res = os.path.split(project)[0]
        res = os.path.join(res,'Resources')
        config['icon'].gen_macos(res)
        config['icon'].gen_ios(res)
