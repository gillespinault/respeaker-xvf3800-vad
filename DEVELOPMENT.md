# ReSpeaker XVF3800 VAD - Notes de développement

## Vue d'ensemble

Ce projet implémente la détection d'activité vocale (VAD) sur le ReSpeaker XVF3800 avec XIAO ESP32-S3, en utilisant ESP-SR VADNet pour la détection et les LEDs du XVF3800 pour le feedback visuel.

## Architecture matérielle

### Composants
- **XIAO ESP32-S3** : Microcontrôleur principal
- **XMOS XVF3800** : Processeur audio (beamforming, AEC, DoA)
- **TLV320AIC3104** : Codec audio (pour speaker, non utilisé actuellement)
- **4 microphones MEMS** : Capture audio
- **12 LEDs RGB** : Feedback visuel (anneau LED)

### Connexions

#### I2C (contrôle XVF3800)
| Signal | GPIO |
|--------|------|
| SDA    | 5    |
| SCL    | 6    |

- XVF3800 : adresse I2C `0x2C`
- AIC3104 : adresse I2C `0x18` (codec audio, non utilisé)

#### I2S (audio)
| Signal | GPIO | Description |
|--------|------|-------------|
| BCK    | 8    | Bit clock |
| WS     | 7    | Word select (LRCLK) |
| DIN    | 43   | Data IN (audio du XVF3800 vers ESP32) |
| DOUT   | 44   | Data OUT (ESP32 vers codec/speaker) |

**Important** : Le XVF3800 fournit un flux audio 32-bit stéréo à 16kHz.

## ESP-SR AFE Configuration

```c
afe_config_t afe_config = AFE_CONFIG_DEFAULT();

// Configuration audio
afe_config.pcm_config.total_ch_num = 2;
afe_config.pcm_config.mic_num = 2;
afe_config.pcm_config.ref_num = 0;  // Pas de référence (AEC fait par XVF3800)
afe_config.pcm_config.sample_rate = 16000;

// VAD
afe_config.vad_init = true;
afe_config.vad_mode = VAD_MODE_3;  // 0=très sensible, 4=peu sensible

// Désactiver les fonctions déjà gérées par XVF3800
afe_config.aec_init = false;
afe_config.se_init = false;
afe_config.wakenet_init = false;
```

### Modes VAD
- `VAD_MODE_0` : Très sensible (beaucoup de faux positifs)
- `VAD_MODE_1` : Sensible
- `VAD_MODE_2` : Modéré
- `VAD_MODE_3` : Peu sensible (recommandé) ✓
- `VAD_MODE_4` : Très peu sensible

## Contrôle des LEDs XVF3800

### Effets disponibles
```c
typedef enum {
    LED_EFFECT_OFF = 0,
    LED_EFFECT_BREATH = 1,
    LED_EFFECT_RAINBOW = 2,
    LED_EFFECT_SINGLE = 3,
    LED_EFFECT_DOA = 4,      // Direction of Arrival ✓
} led_effect_t;
```

### Mode DoA (Direction of Arrival)
Le mode DoA affiche:
- Une **couleur de base** sur toutes les LEDs
- Une **couleur d'indicateur** sur la LED pointant vers la source sonore

```c
xvf3800_set_led_effect(LED_EFFECT_DOA);
xvf3800_set_led_doa_colors(base_color, doa_color);
xvf3800_set_led_brightness(255);
```

### Configuration actuelle
- Base : Bleu (`0x0000AA`) - constant
- DoA silence : Orange (`0xFF6600`)
- DoA parole : Rouge (`0xFF0000`)

## Leçons apprises

### I2S Full Duplex - Problèmes rencontrés

**Tentative échouée** : Jouer un son (TX) tout en enregistrant (RX) sur le même périphérique I2S.

**Problèmes identifiés** :
1. Le DMA I2S fonctionne en mode **circulaire continu** - il boucle le dernier buffer indéfiniment
2. Sur ESP32-S3, TX et RX peuvent être indépendants SAUF si ils partagent les mêmes pins BCK/WS
3. Désactiver TX perturbe les clocks partagés et casse RX

**Solutions possibles (non implémentées)** :
1. Utiliser 2 périphériques I2S séparés avec des clocks différents
2. Alterner entre modes RX et TX (comme dans les exemples Seeed)
3. Écrire du silence en continu sur TX

**Décision** : Abandonner le feedback audio au profit du feedback LED (pas d'interruption de l'enregistrement).

### Codec AIC3104

Le codec TLV320AIC3104 à l'adresse I2C `0x18` doit être initialisé pour que le speaker fonctionne. Code de référence dans `aic3104_codec.c` (non utilisé actuellement).

### Debouncing VAD

Le VAD génère des callbacks très fréquemment (~30ms). Un debouncing est nécessaire :

```c
#define SPEECH_THRESHOLD  5   // ~150ms de parole continue
#define SILENCE_THRESHOLD 15  // ~450ms de silence continu
```

### Commandes I2C et DoA

Les commandes I2C vers le XVF3800 ne doivent PAS être envoyées trop fréquemment - cela perturbe le DoA. Solution : ne mettre à jour les LEDs que sur les **transitions d'état** (après debounce), pas à chaque callback.

## Structure du projet

```
respeaker-xvf3800-vad/
├── main/
│   ├── main.c              # Application principale, callbacks VAD
│   ├── audio_pipeline.c    # I2S RX + ESP-SR AFE
│   ├── audio_pipeline.h
│   ├── xvf3800_i2c.c       # Contrôle XVF3800 via I2C
│   ├── xvf3800_i2c.h
│   └── CMakeLists.txt
├── partitions.csv          # Table de partitions (inclut modèle SR)
├── sdkconfig.defaults      # Configuration ESP-IDF
└── DEVELOPMENT.md          # Ce fichier
```

## Dépendances

Dans `idf_component.yml` :
```yaml
dependencies:
  espressif/esp-sr: "^1.9.5"
```

## Build et Flash

```bash
# Avec ESP-IDF installé
idf.py build flash monitor

# Ou avec le script PowerShell
powershell -ExecutionPolicy Bypass -File run_idf.ps1 build flash monitor
```

## Prochaines étapes possibles

1. **Transcription continue** : Intégrer un modèle de reconnaissance vocale
2. **Streaming audio** : Envoyer l'audio capturé via WiFi/UDP
3. **Wake word** : Activer ESP-SR WakeNet pour un mot de réveil
4. **Optimisation VAD** : Ajuster les seuils selon l'environnement

## Ressources

- [Seeed Wiki - ReSpeaker XVF3800](https://wiki.seeedstudio.com/respeaker_xvf3800_introduction/)
- [ESP-SR Documentation](https://github.com/espressif/esp-sr)
- [ESP-IDF I2S Driver](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/i2s.html)
