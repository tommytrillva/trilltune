# TuneBox

Real-time pitch correction VST3 plugin by Tommy Trill AI.

Snaps vocal audio to a musical key and scale — from subtle natural tuning to hard T-Pain effect via the Retune Speed control.

## Features

- YIN pitch detection with adjustable sensitivity
- 9 scale types (Chromatic, Major, Minor, Pentatonic, Blues, Dorian, Mixolydian, Harmonic Minor)
- Retune Speed: 0 = instant hard-tune, 100 = natural drift
- Dry/Wet mix, Input/Output gain controls
- Dark-themed GUI with real-time pitch meter
- VST3 + AU + Standalone

## Download (Windows)

1. Go to **Actions** tab on this repo
2. Click the latest successful **Build TuneBox VST3** run
3. Download the **TuneBox-VST3-Windows** artifact (zip)
4. Extract and copy `TuneBox.vst3` folder to `C:\Program Files\Common Files\VST3\`
5. Restart your DAW — TuneBox appears in the plugin list

## Build from Source

Requires CMake 3.22+ and a C++17 compiler. JUCE downloads automatically.

### Windows

```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Output: `build\TuneBox_artefacts\Release\VST3\TuneBox.vst3\`

### macOS

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Output: `build/TuneBox_artefacts/Release/VST3/TuneBox.vst3`
Auto-installs to `~/Library/Audio/Plug-Ins/VST3/`

### Linux

```
sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libfreetype6-dev libasound2-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## DAW Setup (Ableton Live)

1. Copy `TuneBox.vst3` to your DAW's VST3 folder
2. Rescan plugins in your DAW preferences
3. Insert TuneBox on a vocal audio track
4. Set Key and Scale to match your song
5. Adjust Retune Speed: 0 for hard-tune, higher for natural correction
