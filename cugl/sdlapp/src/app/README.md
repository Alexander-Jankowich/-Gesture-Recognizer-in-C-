# SDL App Extensions
---
This directory is a collection of extensions that we have found necessary for 
CUGL. Historically this package provided support for safe area queries, though
SDL3 now provides that natively. But with the appearance of large-display
behavior on Android 15+ devices, we need more support for device orienation.

In addition we have included methods to query the device that we find useful for
data analytics. When possible, we try to make sure these are privacy preserving.

Note that these features require modifications to the Android Java files 
(notably `SDLActivity`). Instead of modifying the SDL files, we subclass them 
with a new activity called `APPActivity`.  See the comments in that file in the 
Android project template.