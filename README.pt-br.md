# Protopanda  
<p align="center">
  <img src="doc/logoprotopanda.png" alt="Protopanda">
</p>

**Telegram channel:** https://t.me/mockdiodes
**Telegram chat:** https://t.me/protopandachat

Protopanda é uma plataforma open source (firmware e hardware) para controlar protogens. A ideia é ser simples o suficiente para que você só precise de um pouco de conhecimento técnico para fazê-lo funcionar, mas ao mesmo tempo flexível para que uma pessoa com o mínimo conhecimento de Lua possa fazer quase de tudo.

1. [Features](#features)   
2. [Modelos 3d](#modelos-3d)
3. [Guias](#guias)   
4. [FAQ](#faq)   
5. [Alimentação](#alimentação)  
6. [Painéis](#painéis)  
7. [Tela e Expressões](#tela-e-expressões)  
8. [Compilando o firmware](./doc/flashing-guide.pt-br.md)
9. [Fitas de LED](#fitas-de-led)  
10. [Bluetooth](#bluetooth)  
11. [Hardware](#hardware)   
12. [Montando os eletronicos](#montando-o-seu-protopanda)
13. [Imprimindo e montando as peças](./doc/print-guide.pt-br.md)
14. [Programação em Lua](#programação-em-lua)  

# Features

- Construído sobre o ESP32-S3 N16R8. Facinho de achar
- Animações em 60+ FPS
- Suporte para painel HUB75, matrizes de LED MAX7219 ou matrizes WS2812
- RGB de 16bit
- Suporte para led endereçavel WS2812
- Customização usando Lua
- Expressões faciais são arquivos .PNG
- Utiliza um cartão SD com configurações fáceis de ajustar
- Suporte BLE para controle remoto, ou infra vermelho
- Alimentado via USB-C
- Tela interna com menuzinho
- Modo WiFi, com editorzinho
- Suporte a animação por keyframes com modelos vetoriais
- FFT integrado e animações de boca baseadas em som
- Código aberto e hardware aberto
- Tem jogos!
- gay 🏳️‍🌈

# Modelos 3d

Todos os modelos 3D estão no thingiverse. **IMPORTANTE: o frame frontal está em uma página separada**
**Head:** https://www.thingiverse.com/thing:7188042
**Front frame:** https://www.thingiverse.com/thing:7188045

# Guias

Tem vários guias prontos com imagens e tudo! 

* [Guia de impressão e montagem das peças em 3d](./doc/print-guide.pt-br.md)
* [Fazendo um protopanda do 0 (DIY)](./doc/diy-guide.pt-br.md)
* [Montando oss eletronicos no painel frontal](./doc/front-frame-guide.pt-br.md)
* [Atualizando o firmware e compilando](./doc/flashing-guide.pt-br.md)
* [Configurando seu protogen](./doc/configuring.pt-br.md)
* [Referencia de funções lua](doc/lua-doc.pt-br.md)

# FAQ

[Dê uma olhada no FAQ aqui](./doc/faq.pt-br.md)

# Alimentação  

__TLDR: Use um power bank de pelo menos 20W com PD e USB-C.__

Existem dois modos: um que alimenta diretamente em 5V via USB e outro que possui gerenciamento de energia (conversor buck), que requer de 6,5V até 12V. Este segundo modo só é habilitado através de alterações físicas na PCB.

Cada painel HUB75 pode consumir até 2A no brilho máximo, então alimentar diretamente via USB em 5V pode ser problemático. Por isso, esta versão com o regulador ativa o PD (Power Delivery) no USB, solicitando 9V 3A, o que fornece energia suficiente para ambos os painéis. Infelizmente, esta versão consome muito mais energia.

Na maioria dos casos, você não estará operando os painéis no brilho máximo nem com todos os LEDs em branco, então a versão em 5V é a recomendada.da suporta [tiras de LED](#tiras-de-led), e há uma porta dedicada a elas. A saída também é de 5V, a mesma dos painéis. Como os LEDs são do tipo WS2812B, eles podem consumir até 20 mA por LED a 100% de brilho.  

# Painéis

O recomendado é usar painéis HUB75. Eles são controlados pela [biblioteca hub75 do mrcodetastic](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA), e estes são os [painéis recomendados](https://pt.aliexpress.com/item/4000002686894.html).
![Painéis HUB75](doc/panels.jpg "Painéis HUB75")
Eles são multiplexados, o que significa que apenas alguns LEDs estão ligados por vez. É rápido o suficiente para não ser percebido pelo olho humano. Mas sob luz solar direta, é difícil tirar uma boa foto sem efeito de tearing.
![Tearing capturado na câmera](doc/tearing.jpg "Tearing capturado na câmera")

A resolução é de 64 pixels de largura e 32 pixels de altura. Com dois painéis lado a lado, a área total é de 128x32 pixels. A profundidade de cor é de 16 bits, no formato RGB565, o que significa vermelho (0-32), verde (0-64) e azul (0-32).

Você também pode usar matrizes MAX7219 ou matrizes de LEDs endereçáveis!

# Tela e Expressões  

O Protopanda usa imagens do cartão SD e alguns arquivos JSON para construir as sequências de animação. Todas as imagens devem ser `PNG`; posteriormente, são decodificadas para um formato bruto e armazenadas no [arquivo bulk de quadros](#arquivo-bulk).  

- [Carregando Frames](#carregando-frames)  
- [Expressões](#expressões)  
- [Pilha de Expressões](#pilha-de-expressões)  
- [Arquivo Bulk](#arquivo-bulk)  
- [Modo managed](#modo-managed)  

## Carregando Frames
Para carregar os quadros (frames), você precisa adicioná-los ao cartão SD e especificar suas localizações no arquivo `animation.json`:
```json
{
  "frames": [
    {"pattern": "/expressions/angry/angry%d.png","flip_left": false,"flip_right": true,"from": 5,"to": 9,"name": "frames_angry"},
    {"pattern": "/expressions/angry/angry%d transition.png","flip_left": false,"flip_right": true,"from": 1,"to": 4,"name": "frames_angry_transition"},
    {"pattern": "/expressions/blink/blink%d.png","flip_left": false,"flip_right": true,"from": 1,"to": 8,"name": "frames_blink"},
  ]
}
```

> Modificar o arquivo `animation.json` adicionando ou removendo arquivos forçará o sistema a reconstruir o [arquivo de bulk de frames](#bulk-file).

Cada elemento no array `frames` pode ser tanto o caminho do arquivo quanto um objeto que descreve múltiplos arquivos. [Você pode usar esta página para ajudar.](https://onlinetexttools.com/printf-text)

* **pattern** (string)  
  Assim como no `printf`, que usa `%d` para especificar um número, ao usar `pattern`, é necessário ter os campos `from` e `to`. Por exemplo:  
  Dado o exemplo:
  ```json
  {"pattern": "/bolinha/input-onlinegiftools-%d.png", "from": 1, "to": 155},
  ```
  Isso carregará `/bolinha/input-onlinegiftools-1.png` até `/bolinha/input-onlinegiftools-155.png`.

* **flip_left** (bool)  
  Devido à orientação dos painéis, pode ser necessário inverter o lado esquerdo horizontalmente.

* **flip_right** (bool)  
  Devido à orientação dos painéis, pode ser necessário inverter o lado esquerdo horizontalmente.

* **name** (string)  
  As animações são basicamente como:
  ```
  Desenhar frame 1
  esperar um tempo
  Desenhar frame 2
  ```
  Isso pode ser um problema se você codificar as animações diretamente e precisar adicionar um frame no meio. Para resolver isso, você pode criar um nome para uma imagem ou um grupo de imagens. O nome é apenas um identificador dado ao primeiro frame do `pattern`. Funciona como um offset.

* **color_scheme_left** (string)  
  Se você precisar inverter um ou mais canais de cor, use isso para fazê-lo.
  Qualquer permutação de "rgb", "bgr", "rbg" serve.

## Expressões

Uma vez que os frames são carregados e a execução começa, é trabalho dos [scripts Lua](#programming-in-lua) gerenciar as expressões.  
As expressões são armazenadas em `expressions.json` na raiz do cartão SD.

```json
{
  "frames": [],
  "expressions": [
    {
      "name": "normal",
      "frames": "frames_normal",
      "animation": [1, 2, 1, 2, 1, 2, 3, 4, 3],
      "duration": 250,
      "overlay": "mouth"
    },
    {
      "name": "sus",
      "frames": "frames_amogus",
      "animation": "auto",
      "duration": 200
    },
    {
      "name": "noise",
      "frames": "frames_noise",
      "animation": "loop",
      "duration": 5,
      "onEnter": "ledsStackCurrentBehavior(); ledsSegmentBehavior(0, BEHAVIOR_NOISE); ledsSegmentBehavior(1, BEHAVIOR_NOISE)",
      "onLeave": "ledsPopBehavior()"
    },
    {
      "name": "boop",
      "frames": "frames_boop",
      "animation": [1, 2, 3, 2],
      "duration": 250
    },
    {
      "name": "boop_begin",
      "frames": "frames_boop_transition",
      "animation": [1, 2, 3],
      "duration": 250,
      "transition": true
    },
    {
      "name": "boop_end",
      "frames": "frames_boop_transition",
      "animation": [3, 2, 1],
      "duration": 250,
      "transition": true
    }
  ],
  "scripts": [],
  "boop": {}
}
```

#### Expression Properties  

- **`name`** (string, *optional*)  
  Não é obrigatório ter um nome, mas é uma forma de facilitar a chamada de uma animação e visualizar seu nome no menu.

- **`frames`** (string)  
  O nome do grupo de frames que contém os frames desejados.

- **`animation`** (int[] or `"auto"`)  
  - `int[]`: O ID de cada frame a ser exibido (e.g., `[1, 2, 3]`).  
  - `"loop"`: Quando definido como "loop", os frames serão adicionados automaticamente em sequência. (e.g., `1, 2, 3...`).  
  - `"pingpong"`: Quando definido como "pingpong", os frames serão adicionados automaticamente em sequência depois repetindo invertido. (e.g., `1, 2, 3... ...3, 2, 1`).  
  - `"loop_backwards"`: Mesmo comportamdno do loop, porém de trás pra frente (e.g., `...3, 2, 1`).  

- **`duration`** (int)  
  A duração de cada frame.

- **`hidden`** (string)  
  Hide from menu selection 

- **`intro`** (string)  
  Esse parâmetro deve ser o nom e de uma outra animação que DEVE ser uma `tranistion=true`. Essa animação de introdução irá tocar sempre que essa expressão entrar

- **`outro`** (string)  
  Esse parâmetro deve ser o nom e de uma outra animação que DEVE ser uma `tranistion=true`. Essa animação de encerramento irá tocar sempre que essa expressão sair de cena

- **`transition`** (boolean)  
  Transforma no tipo `transition`. Isso fará com que essa animação só toque uma vez e quando chamada, seja inserida na pilha de animações sem subistituir a atual. Apenas tocar uma vez e voltar para a anterior

- **`repeats`** (int, default 1)
  Se a expressão é do tipo `transition`, você pdoe fazer com que ela se repita N vezes

- **`overlay`** (string)
  Nome da sobreposição (overlay) a ser usada nessa animação

- **`onEnter`** (string, Lua code)  
  Quando a animação assume o controle da tela, executa um código Lua.

- **`onLeave`** (string, Lua code)  
  Quando a animação para de executar (por estar marcada como `transition=true` ou porque outra animação assumiu o controle), executa um código Lua.

## Sobreposições (Overlays)

Às vezes você quer algo com um pouco mais de estilo. Como uma boca que se move enquanto você fala, ou algumas estrelas, ou até mesmo algo que reaja a um acelerômetro. Para isso, você pode criar sobreposições:

```json
{
"overlays"   : [
    {
      "name"    : "stars",
      "elements": [
        {
          "sprites"           : [
            "/expressions/overlays/star.png"
          ],
          "transparency": true,
          "transparency_color": "#ff00ff",
          "animation"         : {
            "mode": "random_flashing",
            "alive_duration": 50,
            "interval_min": 100,
            "interval_max": 500,
            "min_x": 0,
            "max_x": 64,
            "min_y": 0,
            "max_y": 64
          }
        }
      ]
    }
    {
      "name"    : "mouth",
      "elements": [
        {
          "sprites"           : [
            "/expressions/overlays/mouth0.png",
            "/expressions/overlays/mouth1.png",
            "/expressions/overlays/mouth2.png",
            "/expressions/overlays/mouth3.png",
            "/expressions/overlays/mouth4.png"
          ],
          "transparency": false,
          "transparency_color": "#000000",
          "animation"         : {
            "mode": "fft",
            "x": 11,
            "y": 19,
            "band_start": 2,
            "band_end": 8,
            "attack": 0.05,
            "release": 0.2,
            "min_energy": 50000,
            "max_energy": 200000,
            "frist_frame_threshold": 60000
          }
        }
      ]
    }
  ]
}
```

## Pilha de Expressões  

As expressões são armazenadas em uma pilha. Quando você adiciona uma animação que não se repete, ela pausa a animação atual e executa até o final da nova animação. Se você adicionar duas ao mesmo tempo, a última será executada. Quando terminar, a anterior será executada.  

## Arquivo Bulk  

Mesmo com o cartão SD, mudar os quadros não é tão rápido. A interface do cartão SD não é rápida o suficiente. Para acelerar, as imagens são decodificadas de PNG para dados brutos de pixels no formato RGB565 e armazenadas na flash interna. Todos os quadros são armazenados em um único arquivo chamado `arquivo bulk`. Isso é feito de forma que os quadros são armazenados sequencialmente e, mantendo o arquivo aberto, a velocidade de transferência é acelerada, atingindo 60 FPS.  

Toda vez que você adiciona ou modifica um novo quadro, é necessário reconstruir esse arquivo. Isso pode ser feito no menu ou chamando a função Lua `composeBulkFile`.  

## Modo managed

As animações são processadas pelo Núcleo 1, então você não precisa perder tempo precioso nos [scripts Lua](#programação-em-lua) atualizando-as. É possível mudar o quadro usando scripts Lua... Mas também é um desperdício. Então deixe isso para o outro núcleo, e você só precisa se preocupar em selecionar quais expressões deseja!  

Durante o modo gerenciado, o desenho dos quadros é tratado pelo Núcleo 1.  
![alt text](mdoc/managed.png "Modo Gerenciado")  

# Compilando

O guia completo está aqui: [Compilando o firmware](./doc/flashing-guide.pt-br.md)

# Fitas de LED  

O Protopanda suporta o protocolo de LED endereçável WS2812B e fornece um sistema simples para definir alguns comportamentos para a fita/matriz.  
![alt text](doc/A7301542.JPG)  
![alt text](doc/ewm.drawio.png)  

Você pode defini-los dentro do `hardware.json`:
```json
{
  "leds": { 
    "pin_mode": "double",
    "_comment": "Modos permitidos: 'double' e 'single'. Se usar fitas de LED extras, todas serão conectadas ao pino de LED direito se 'double' estiver definido",
    "groups":[
      {
        "_comment": "Se pin_side não estiver definido, o padrão é 'left'",
        "pin_side": "left",
        "led_count": 64,
        "mode": "pride"
      },
      {
        "pin_side": "right",
        "led_count": 64,
        "mode": "pride"
      }
    ]
  }
}
```

### Modos Disponíveis e Parâmetros

| Modo | Descrição | Parâmetros |
|------|-------------|------------|
| `none` | LEDs permanecem apagados | Nenhum |
| `pride` | Animação do arco-íris pride | Nenhum |
| `rotate` | Cor rotativa ao longo da fita | `speed` (ms) - velocidade de rotação |
| `random_color` | Cada LED pisca em cores aleatórias | Nenhum |
| `fade_cycle` | Ciclo gradual de cores | `hue` (0-255), `speed` (ms), `min_brightness` (0-255) |
| `rotate_fade_cycle` | Ciclo fade com rotação | `hue`, `speed`, `min_brightness`, `rotate_speed` (ms) |
| `color_rgb` | Cor RGB estática | `r` (0-255), `g` (0-255), `b` (0-255) |
| `color_hsv` | Cor HSV estática | `h` (0-255), `s` (0-255), `v` (0-255) |
| `random_blink` | LEDs piscam aleatoriamente | `base_hue` (0-255), `hue_variance` (0-255), `brightness` (0-255), `blink_speed` (ms) |
| `icon_x` | Exibe um padrão "X" | Nenhum |
| `icon_y` | Exibe um padrão "Y" | Nenhum |
| `icon_v` | Exibe um padrão "V" | Nenhum |
| `rotate_sine_v` | Variação de brilho em onda senoidal | `hue` (0-255), `saturation` (0-255), `speed` (ms) |
| `rotate_sine_s` | Variação de saturação em onda senoidal | `hue` (0-255), `brightness` (0-255), `speed` (ms) |
| `rotate_sine_h` | Variação de matiz em onda senoidal | `sat` (0-255), `brightness` (0-255), `speed` (ms) |
| `fade_in` | Efeito de fade-in gradual | `hue` (0-255), `saturation` (0-255), `step` (0-255), `delay` (ms) |
| `noise` | Efeito de ruído aleatório | `step` (0-255), `delay` (ms) |

# Bluetooth  

Des da versão 2.0, o protopanda suporta quase qualquer dispositivo BLE (bluetooth low energy) que tenha HID. Porém, é possivel criar 'drivers' usando Lua. Por padrão, o protopanda suporta:
* https://github.com/mockthebear/ble-fursuit-paw
* https://pt.aliexpress.com/item/1005008459884910.html
* https://pt.aliexpress.com/item/1005009845485445.html

Porém teoricamente, um joystick que roda em low energy deve ser suportador atraves de keybinds

## Keybind

Atualmente as keybinds padrão são:
```json
{
  "keybinds":{
    "joystick.right_hat=5": "BUTTON_LEFT",
    "joystick.right_hat=3": "BUTTON_DOWN",
    "joystick.right_hat=1": "BUTTON_RIGHT",
    "joystick.right_hat=7": "BUTTON_UP",
    "joystick.buttons.4": "BUTTON_CONFIRM",
    "joystick.buttons.1": "BUTTON_BACK",

    "beauty.buttons.4": "BUTTON_LEFT",
    "beauty.buttons.1": "BUTTON_DOWN",
    "beauty.buttons.3": "BUTTON_RIGHT",
    "beauty.buttons.2": "BUTTON_UP",
    "beauty.buttons.5": "BUTTON_CONFIRM",
    "beauty.buttons.6": "BUTTON_BACK"

  }
}
```

Todas as keybinds mapeiam para input do controle do protopanda.

# Hardware  

O Protopanda foi projetado para rodar no ESP32-S3-N16R8, uma versão com 16 MB de flash, 384 kB de ROM, 512 kB de RAM e 8 MB de PSRAM octal. É necessário essa versão com mais espaço e PSRAM para ter RAM suficiente para rodar os painéis, BLE e Lua simultaneamente.  

No hardware, há uma porta para os dados HUB75, um conector para cartão SD, dois terminais parafusados para a saída de 5V, os pinos de alimentação, uma porta I2C e um pino para tira de LED.  

## Diagrama  

![Diagrama](doc/noitegrama.png "Diagrama")  

## Portas  

![Portas](doc/ports.png "Portas")  

## Esquemático  

![Diagrama](doc/schematic.png "Diagrama")  

## Dois Núcleos  
O Protopanda utiliza e abusa dos dois núcleos do ESP32.  
* **Núcleo 0**  
Por padrão, o Núcleo 0 é projetado principalmente para gerenciar o Bluetooth. Quando não está fazendo isso, ele gerencia as animações e, quando o [Modo Gerenciado](#modo-gerenciado) está ativo, também cuida da atualização da tela de LED.  
* **Núcleo 1**  
O segundo núcleo lida com tarefas não relacionadas à tela. Ele possui a rotina que verifica o [nível de energia](#alimentação), atualiza as entradas, lê os sensores e chama a função Lua `onLoop`.  

# Montando o seu protopanda

Sei que fazer uma PCB do zero, usar componentes SDM é complicado. Porém, você pode usar peças que dá para comprar no aliexpresse montar uma versão reduzida do protopanda.

Pois bem, existe um [guia para montar o seu próprio protopanda!](./doc/diy-guide.pt-br.md)


![Diagrama](doc/diy-schematic.png "Esquema elétrico")  


# Imprimindo e montando as peças 
[Guia aqui](./doc/print-guide.pt-br.md)

# Programação em Lua  

__[Lua functions reference](doc/lua-doc.pt-br.md)__

- [Script Lua Mínimo](#script-lua-mínimo)  
- [Ciclar Expressões a Cada Segundo](#ciclar-expressões-a-cada-segundo)  

## Script Lua mínimo  
```lua  
-- Script Lua mínimo em init.lua  

function onSetup()  
  -- Esta função é chamada uma vez. Aqui você pode iniciar o BLE, começar a escanear, configurar o painel, definir o modo de energia, carregar bibliotecas e preparar as tiras de LED e até ligar a energia.  
  -- Todas as chamadas aqui são feitas durante o SETUP, rodando no núcleo 0.  
end  

function onPreflight()  
  -- Aqui, todas as chamadas Lua são feitas a partir do núcleo 1. Você pode até deixar esta função em branco.  
  -- O Núcleo 0 só começará a gerenciar após 100 ms (o bip final).  
end  

function onLoop(dt)  
  -- Esta função será chamada em loop.  
  -- O parâmetro dt é a diferença em MS entre o início do último quadro e o atual. Útil para armazenar tempo decorrido.  
end  
```  

## Ciclar expressões a cada segundo  
```lua  
local expressions = dofile("/lualib/expressions.lua")  
local changeExpressionTimer = 1000 -- 1 segundo  

function onSetup()  
  setPanelMaxBrightness(64)  
  panelPowerOn() -- O brilho sempre começa em 0  
  gentlySetPanelBrightness(64)  
end  

function onPreflight()  
  setPanelManaged(true)  
  expressions.Next()  
end  

function onLoop(dt)  
  changeExpressionTimer = changeExpressionTimer - dt  
  if changeExpressionTimer <= 0 then  
    changeExpressionTimer = 1000 -- 1 segundo  
    expressions.Next()  
  end  
end  
```  
