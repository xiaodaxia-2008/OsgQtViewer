# OsgQtViewer

```shell
sudo apt install fontconfig-config libopenscenegraph-3.4-dev qt5-default qttools5-dev
```

# Build

```shell
mkdir Build && cd Build
cmake ..
make -j
```

run example `./bin/OsgQtViewer.cpp`

# Package
Run `cpack` in `build` folder will create a Debian `.deb` package. Copy the `.deb` package to other Debian machine and run `sudo dpkg -i xxxx.deb` to install the OsgQtViewer. Currently this only install a `OsgQtViewer` to `/usr/local/bin` directory. So you can launch the demo with `OsgQtViewer` in your terminal.
