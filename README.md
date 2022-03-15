<h1 align="center">Bot for World of Tanks: Strategy</h1>

<p align="center">

<img src="https://img.shields.io/badge/made%20by-Geschwader-green" >

<img src="https://img.shields.io/badge/platform-windows-lightgrey">

<img src="https://badges.frapsoft.com/os/v1/open-source.svg?v=103" >

<img src="https://img.shields.io/badge/status-down-green">

</p>

<p align="center">
  
<h2 align="center">:warning: <b>The bot is not ready yet, it is being updated</b></h2>
  
</p>

## Table of Contents
- [Required components](#required-components)
- [About the project](#about-the-project)
- [Creating a project using CMake](#creating-a-project-using-cmake)
- [Compiling the code and testing bot](#compiling-the-code-and-testing-bot)
- [Future plans](#future-plans)
- [Main Developers](#main-developers)
- [Thanks](#thanks)
- [Support us](#support-on-coffee-pay)


## Required components:
- **Git**
- **Cmake**
- **Visual Studio 2019**
- **С++ 17** 

## About the project

### Internal implementation

- To work with json, the project uses a popular library [nlohmann/json](https://github.com/nlohmann/json)
- The Winsock C++ library is used to work with the server.

### Runtime

- You just need to enter the name and the game and the bot will do everything for you.

## Creating a project using CMake
### Command Line / Terminal required
Create a folder build in Geschwader:
```
> mkdir build
```
Now go to our `Geschwader/build` folder and issue the following command to generate a project using CMake:
```
> cmake .. -G "Visual Studio 16 2019"
```
## Compiling the code and testing bot
Now once the project is created, we can easily compile it from the console from the build directory using the command:
```
> cmake --build .
```
Open the `build\CMakeBot\Debug` folder and type the following command for launch server:
```
> CMakeBot.exe
```

Now you can enter the name and game and start the bot.

## Future plans

- Improve project

## Main Developers

- [Daniil Telegin](https://t.me/LittleFantom)
- [Anton Poblitsa](https://t.me/podikgg)
- [Roman Zhulyanov](https://t.me/grangeli)

## Thanks

We thank the WG Forge team for their invaluable knowledge and experience, as well as for round-the-clock support in creating the project.

## Support on Coffee pay

Hey dude! Help us out for couple of cups of coffee!

### __It may be added later__
