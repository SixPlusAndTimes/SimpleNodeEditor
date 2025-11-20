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

[core-nodes](https://github.com/onurae/core-nodes)

# TODOS

- [x] port口名字显示时排版美化
- [x] pipeline name 编辑和保存
- [x] Node Edge 属性的显示和编辑功能 
- [x] node Edge 的裁剪功能
- [x] zoom in out 功能
- [x] 手动toposort的功能，添加用户输入处理的逻辑？ 按下 S 键， 重新toposort
- [x] file drop 功能
- [ ] 截屏功能?
- [ ] 可能的bug ： Edge已经被裁掉，此时它连接的Node添加了一个新的PruningRule，Edge无法同步新的pruningrule
- [ ] 重复代码太多了
- [ ] notify 组件
- [x] node property 正确显示以及添加新的property
- [ ] log 封装
- [ ] 如果nodedes中没有yamlpipeline中的对应的portid，别挂死，提示错误即可 
- [ ] 允许不显示没有Edge连接的port
- [ ] 主动删除当前pipeline的用户交互
- [ ] hover 在port上能够显示portid
- [ ] 自定义node颜色
- [ ] 支持多个窗口切换，分别渲染不同的pipeline
- [ ] bugs: add node popup 不能滑动到底部 
