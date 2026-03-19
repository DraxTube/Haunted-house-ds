# Haunted House DS 👻

Un porting homebrew di **Haunted House** (Atari 2600, 1982) per **Nintendo DS**.

Il binario `.nds` è distribuibile liberamente. La ROM originale **non è inclusa**: devi portarla tu sulla scheda SD.

---

## Come usarlo

### Sul flashcart (hardware reale)

1. Scarica `haunted_house_ds.nds` dalla sezione [Releases](../../releases).
2. Copia il file `.nds` nella root della tua scheda SD.
3. Crea la cartella `data/` nella root della SD.
4. Copia la tua ROM Atari 2600 come `data/haunted_house.a26` (deve essere esattamente **4096 byte**).
5. Lancia `haunted_house_ds.nds` dal menu del tuo flashcart (R4, Acekard, ecc.).

### Struttura SD card

```
SD:/
├── haunted_house_ds.nds
└── data/
    └── haunted_house.a26   ← la tua ROM qui
```

---

## Controlli

| Tasto DS | Funzione Atari |
|----------|----------------|
| D-Pad    | Joystick       |
| A / B    | Fuoco (pick up / use item) |
| START    | Game Reset     |
| SELECT   | Game Select    |
| L        | Difficulty A   |
| R        | Difficulty B   |

---

## Come compilare

### Requisiti

- [devkitPro](https://devkitpro.org/wiki/Getting_Started) con `nds-dev`
- `make`

### Compilazione locale

```bash
# Installa devkitPro (Linux)
wget https://apt.devkitpro.org/install-devkitpro-pacman
chmod +x install-devkitpro-pacman
sudo ./install-devkitpro-pacman
sudo dkp-pacman -S nds-dev

# Esporta variabili
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM

# Compila
make
```

Il file `haunted_house_ds.nds` viene creato nella root del progetto.

### GitHub Actions (automatico)

Ogni push su `main` o `master` compila automaticamente il progetto tramite GitHub Actions e pubblica il `.nds` come artifact scaricabile dalla tab **Actions**.

Per creare una release pubblica, tagga un commit:
```bash
git tag v1.0.0
git push origin v1.0.0
```

---

## Architettura tecnica

```
src/
├── main.c       Entrypoint: init, game loop, caricamento ROM
├── emu6502.c/h  Emulatore MOS 6507 (CPU Atari 2600)
├── tia.c/h      Emulazione TIA (chip grafico/audio Atari)
├── riot.c/h     Emulazione RIOT 6532 (timer, I/O joystick)
├── video.c/h    Rendering: TIA → schermo DS (scala 160×192 → 256×192)
└── input.c/h    Mapping tasti DS → joystick/switch Atari
```

Il flusso è:
1. La ROM viene caricata da `fat:/data/haunted_house.a26`
2. Ogni frame: il core 6502 esegue ~19950 cicli (timing NTSC 60Hz)
3. Il chip TIA tiene traccia dei registri grafici scritti dalla ROM
4. `video.c` converte lo stato TIA in pixel sul top screen del DS
5. I tasti DS aggiornano i registri RIOT (joystick + switch console)

---

## Note legali

La ROM di Haunted House è copyright **Atari SA**. Questo progetto non la include né la distribuisce. Usa esclusivamente una ROM di tua proprietà.

Il codice sorgente di questo porting è rilasciato sotto licenza **MIT**.

---

## Crediti

- Haunted House originale: James Andreasen / Atari (1982)
- devkitPro / libnds: devkitPro team
- Porting DS: [il tuo nome qui]
