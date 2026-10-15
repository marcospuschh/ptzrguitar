# Aurora Guitar Studio (VST3 / AU / Standalone)

Cadeia: Input (mono/stereo) > Gate > Guitar voicing > Pedais (2 opto, 2 octave, 2 drive)
> [efeitos antes do amp] > Amp A (+ Amp B em dual) com cabine e microfone > [efeitos depois do amp] > Output.

- 4 amps: Silverface (clean), Class A 30, Boutique, British Crunch
- 4 cabines + IR customizado, 5 microfones, mic arrastavel sobre o speaker
- Modulacao: chorus, vibrato, tremolo | Delay: digital, analog, tape, ping-pong, dual (+ tap tempo)
- Reverb: room, hall, plate, spring, shimmer, cloud
- Afinador, tap tempo, medidor, gate, 6 vozes de guitarra, tema claro/escuro

## Gerar o .dmg (GitHub Actions)
1. Suba todo o conteudo desta pasta para o repositorio (inclui .github/workflows/build.yml).
2. Aba Actions: espere o build terminar e baixe AuroraGuitarStudio-mac.

## Instalar no Mac
- VST3: ~/Library/Audio/Plug-Ins/VST3/    AU: ~/Library/Audio/Plug-Ins/Components/
- Como nao e assinado pela Apple, rode uma vez:
  xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/VST3/Aurora Guitar Studio.vst3"
  (o mesmo para o .component) e refaca o scan de plugins na DAW.

## Compilar localmente
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
