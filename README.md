# OsgQtViewer

```shell
sudo apt install fontconfig-config libopenscenegraph-3.4-dev
```

# Build

```shell
mkdir Build && cd Build
cmake ..
make -j
```

run example `./bin/BasicViewer.cpp`

# Package
Run `cpack` in `build` folder will create a Debian `.deb` package. Copy the `.deb` package to other Debian machine and run `sudo dpkg -i xxxx.deb` to install the OsgQtViewer. Currently this only install a `BasicViewer` to `/usr/local/bin` directory. So you can launch the demo with `BasicViewer` in your terminal.
