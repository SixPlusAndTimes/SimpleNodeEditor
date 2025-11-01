# Simple Node Editor
## notice
**only tested on windows and wsl**

**all libs built from source**

**this starter project use opengl3 and sdl2 as imgui's backends， and ourown diretory uses the corresponding example from imnode*

## how to build
following commads will make debug target bydefault

~~~shell
cd ImguiCmakeStarter
mkdir cmake_build
cmake -S . -B .\cmake_build\
cmake --build .\cmake_build\ 
~~~~

Default intallation path is "${CMAKE_CURRENT_SOURCE_DIR}/install", you can check it in CmakeLists.txt.  

Note we should use --config Debug option here, because cmake intall command find Release target by default but if we complie the Debug one only, the following installation command step will complain

~~~shell
cmake --install .\cmake_build\ --config Debug 
~~~

If you want intall it at another dir, just modify the default intallation path in CmakeLists.txt, or overwrite it in command line option using `-DINSTALL_PREFIX="custom_install"`

# references
[sdl2](https://github.com/libsdl-org/SDL)

[imgui](https://github.com/ocornut/imgui)

[imnode](https://github.com/Nelarius/imnodes)


[imnode zoom](https://github.com/Nelarius/imnodes/pull/192)  zoom feature is copied from here

# TODOS

- port口名字显示时排版美化
- Node Edge 属性的显示和编辑功能 
- node Edge 的裁剪功能
    - 目前只能处理完全从yaml文件解析出来的pipeline，如果手动添加edge可能会crash，以后再修复
    - bug原因：restore edge时要借助yamlnode属性，但是手动添加的edge没有yaml属性, 需要自动生成yamlnode、links 属性
        - 但是这样的话NodeEditor也需要保证yamlnodeid的唯一性
- zoom in out 功能
- 手动toposort的功能，添加用户输入处理的逻辑？ 按下 S 键， 重新toposort?
- file drop 功能
- 截屏功能?
- node 如果即将被裁剪，需要检查它的edge是否也被裁剪掉了，如果不是不进行prue rule的切换，而是发出提醒需要填写相应的prune rule
    - 那么首先得完成属性的显示和编辑功能