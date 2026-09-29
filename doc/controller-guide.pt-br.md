# Controle remoto

Protopanda **EXIGE** que você tenha uma forma de controlá-lo. Sim, dá para alternar as animações apertando o botão "boot" (GPIO 0), e dá para gambiarrar algum botão nos GPIOs disponíveis... Mas né. Meio zoado.

Tem o [app pra Android](https://play.google.com/store/apps/details?id=gay.protopanda.controller). Quebra o galho já que você não precisa construir nem comprar nada extra, mas durante uma convenção é meio desajeitado, né? Por isso estamos aqui para construir um controle!

Diferente do controlador do Protopanda, aqui tem bastante margem para errar e **muito** espaço para personalizar.

## Variações

Atualmente o código suporta estes microcontroladores:

* NRF52840
* NRF52832
* Esp32 SuperMini

Não recomendo o ESP32 porque ele come muita energia e precisa de uma bateria forte, mas é uma plataforma que a maioria dos makers já usou antes.
O MCU ideal seria o NRF52832, é um chip que mal consome energia e que pode durar semanas/meses em deep sleep com uma única bateria CR2032, mas exige que você consiga um j-link.
O meio-termo é o NRF52840, usa um pouco mais de energia que o NRF52832, mas ainda dura dias com uma única CR2032 e não precisa do j-link.

## O que vamos construir

Resumindo, é isto:

![alt text](guide-controller-1.png)

Uma bateria, uma liga-desliga, o controlador, um acelerômetro e 6 botões. Só isso.

## Materiais

Neste guia, usamos o NRF52840. Você pode escolher outro, desde que ele tenha BLE e você ajuste os pinos de acordo.

### Variantes do NRF52840

Existem 3 variantes. As fotos do guia cobrem a variante mais barata. Mas as outras duas funcionam direto, basta soldar os pinos corretos.

#### Mais barata

[Nice!Nano V2.0](https://s.click.aliexpress.com/e/_c3xJ6Eat)

![alt text](guide-controller-2.png)

Esta placa é a maior em comparação com as outras duas variantes. Mas é de longe a mais barata.

#### Pequeno só que mais caro

[XIAO nRF52840 Plus](https://s.click.aliexpress.com/e/_c4FMNzzF)

![alt text](guide-controller-3.png)

Menos da metade do tamanho da Nice!Nano. Mais cara por ser da SeeedStudio.

#### MELHOR opção, mas mais cara

[XIAO nRF52840 Sense Plus](https://s.click.aliexpress.com/e/_c4FMNzzF)

![alt text](guide-controller-3.png)

Igual à outra, mas esta versão já vem com um acelerômetro embutido. O projeto usa esse mesmo acelerômetro, então é uma vitória!
Se você escolher esta, pode pular completamente a soldagem do LSM6DS3.

### Lista de materiais

> Leia o guia inteiro antes de comprar qualquer coisa.

* O controlador de sua escolha; neste guia, a [Nice!Nano V2.0](https://s.click.aliexpress.com/e/_c3xJ6Eat)
* [LSM6DS3, se não estiver usando a XIAO Sense Plus](https://pt.aliexpress.com/item/1005012449925493.html)
* 6x [botão tátil com 2 pinos](https://pt.aliexpress.com/item/1005007173900553.html). Sinceramente, QUALQUER botão serve, escolha o que achar melhor
* [Case de CR2032 com chave](https://s.click.aliexpress.com/e/_c3V47s1r). Lembra de comprar uma bateria extra também.
* [Fios de silicone flexíveis](https://pt.aliexpress.com/item/1005008153169841.html). Pode ser qualquer fio, eu só gosto dos de silicone por serem resistentes.
* [J-LINK, somente se usar o NRF52832](https://s.click.aliexpress.com/e/_c40sFFM1) (não será usado neste guia)
* [Luvas finas de tecido](https://s.click.aliexpress.com/e/_c2ui0zi9)

## Gravando o firmware

O primeiro passo é gravar o firmware para testar a placa. Para isso, siga o guia [Configurando o ambiente](./flashing-guide.pt-br.md#configurando-o-ambiente) até a parte de gravar. Você vai precisar do pioarduino/PlatformIO.
Depois, no Visual Studio Code, adicione o workspace do controle remoto. A pasta fica dentro da pasta do protopanda, em `remote-control\nrfversion`.

Quando carregar, você precisará selecionar o ambiente correto. Neste guia estamos usando a Nice!Nano, então clique nesta opção:

![alt text](guide-controller-4.png)

Uma janela vai abrir e você deve selecionar `nice_nano`

![alt text](guide-controller-5.png)

Espere o VS Code terminar de configurar e baixar todas as ferramentas. Quando estiver pronto, conecte a placa no computador. Ela pode aparecer como um dispositivo de armazenamento; se não aparecer, sem problemas, também está ok.

![alt text](guide-controller-6.png)

Então clique em upload. Ele vai compilar o projeto e, por fim, gravar. 

![alt text](guide-controller-7.png)

Pode falhar na primeira vez. Aperte de novo, talvez tire do USB e conecte outra vez; uma hora vai funcionar, a não ser que a placa tenha vindo bichada, ai é lixo.

Depois de gravado, você verá um LED vermelho piscar 5 vezes e depois ficar aceso.
Se estiver usando a XIAO, são dois LEDs piscando, um vermelho e um azul, e depois o azul fica aceso.

Essa piscada ao ligar indica que o controle não encontrou o IMU e vai rodar sem transmitir os dados do acelerômetro. Já dá para testar.

Ligue o seu protopanda e garanta que ele esteja em modo de pareamento

![alt text](guide-controller-8.png)

Então ele deve conectar ao protopanda. O ícone [X] vai mudar e o LED vermelho do NRF vai piscar de vez em quando.

Pronto. Você colocou o controle para funcionar, só falta adicionar alguns botões e o acelerômetro!

## Montagem

Para todas as partes a seguir, sugiro baixar o app Android ou usar outro controle para navegar até um ponto específico do menu.

No menu principal, vá em `scripts` e procure por `control test`. Fique nesse script com o seu protopanda ligado para poder testar.
Garanta que você já pareou o controle ao menos uma vez antes, porque não é possível ativar o modo de pareamento enquanto estiver dentro de um script.

Se você não consegue fazer nenhuma dessas coisas por falta de um controle, edite o seu init.lua.
Encontre a função `onPreflight` e, no final dela, adicione isto:

```lua
scripts.StartScript(8)
```
O número 8 pode mudar: abra o seu `misc.json`, procure a seção `scripts` e conte a partir de 1 até encontrar o
```json
        {
        "name": "Controls Test",
        "file": "/scripts/controls.lua"
        },
```
Se for o 8, tudo certo. Se for o 7, troque o 8 por 7.

Ao ligar, você deve ver a janela do control test.

![alt text](guide-controller-11.png)

### IMU

Lembre-se: se você tem a XIAO Sense Plus, pode pular esta parte!

![alt text](guide-controller-9.png)

Você precisará ligar os fios como no esquema acima.

![alt text](guide-controller-10.png)

Quando você ligar o controle agora, ele não deve piscar 5 vezes e, no script Control Test, você deve ver uma linhazinha se mexendo conforme você balança o acelerômetro.

![alt text](guide-controller-12.gif)

### Bateria

![alt text](guide-controller-14.png)

Uma única CR2032, que alguns conhecem como "pilha de placa-mãe", tem mais do que energia suficiente para alimentar esse carinha por bastante tempo. Então vamos adicionar esse suporte com fios e uma chave!

![alt text](guide-controller-13.png)

Quando você colocar uma pilha e ligar a chave, ele deve ligar e conectar ao protopanda.
**Cuidado para não colocar a pilha com a orientação errada!**

### Teclado de botões

O teclado de botões aqui é só uma sugestão.
Quer que o seu controle seja como um controle remoto de TV? Vai fundo!
Que tal um botão por dedo? Vai fundo!
Dá para trocar os botões por reed switches? Dá! Vai ficar estranho pra caramba, mas dá.

**Tudo o que você precisa para marcar um botão como "pressionado" é ligar o GND ao GPIO correto.**

![alt text](guide-controller-15.png)

Alternativamente, se você estiver usando a XIAO Sense Plus

![alt text](guide-controller-16.png)

Neste guia, vou fazer um tecladinho que fica preso no polegar. Vamos começar por ele. Você também vai precisar de um pedaço de placa perfurada.

![alt text](guide-controller-17.png)

Organize os botões nesta ordem

![alt text](guide-controller-18.png)

Depois precisamos fazer a ponte de todos os GNDs do outro lado

![alt text](guide-controller-19.png)

![alt text](guide-controller-20.png)

Existe uma orientação e uma ordem corretas para ligar os fios.

![alt text](guide-controller-21.png)
![alt text](guide-controller-22.png)

Com tudo ligado, ligue o controle e confira que nenhum dos quadradinhos está preenchido. Aperte alguns botões: cada um deles deve acender um, e apenas um, dos quadradinhos.
Depois confira se você ligou os fios corretamente apertando em ordem como neste gif:

![alt text](guide-controller-23.gif)

Quando estiver completo, você pode remover as bordas e o excesso de placa ao redor com um alicate de corte ou uma dremel.

![alt text](guide-controller-24.png)

### Colando na luva

> Nesta etapa, garanta que nenhum fio passe por cima ou muito perto da antena.

![alt text](guide-controller-26.png)

As mãos se mexem muito, e o ponto mais fraco de tudo o que fizemos é onde o fio encontra a solda. Então precisamos aliviar a tensão nele.
Para isso, vamos começar pelos fios do controlador. Aplique um pouco de cola quente no lado interno da PCB e dobre os fios sobre ela. Se necessário, aplique um pouco por cima também.

![alt text](guide-controller-25.png)

Faça isso com todos os fios, se possível. Caso contrário, eles vão quebrar em menos de um dia de uso.

Esta parte agora é complicada: você pode querer imprimir uma mão em 3D ou encher a luva com alguma coisa. Pessoalmente, gosto de usar a minha própria mão nesta etapa, fica melhor, mas exige alguma destreza.

Com a luva na mão, coloque um pouco de cola quente (não muito quente, cuidado para não se queimar) no polegar e cole o tecladinho ali.

![alt text](guide-controller-27.png)

Passe os fios entre o polegar e o indicador e posicione o controlador nesta orientação. Adicione cola quente para segurar no lugar, só um pouquinho por enquanto.

![alt text](guide-controller-28.png)

Quando esfriar, adicione cola aos poucos no tecladinho, no controlador e no acelerômetro até que fiquem bem presos.

> CUIDADO PARA NÃO DERRAMAR COLA QUENTE NA PORTA USB

Agora, se você deixar assim, vai haver muita tensão em um único ponto do fio, e os fios vão ficar espalhados por todo lado. Para evitar isso, vamos adicionar alguns alívios de tensão e pontos de ancoragem aos fios.

Como na foto, adicione um pingo de cola quente, arrume os fios sobre ele, adicione um pouco por cima e segure no lugar enquanto esfria:

![alt text](guide-controller-29.png)

Faça isso em pelo menos 3 pontos do fio do tecladinho e em dois para os fios da bateria.

![alt text](guide-controller-30.png)

Garanta que todos os fios ligados à PCB tenham um ponto de alívio de tensão e que, quando você mexer a mão, **nenhum fio nas soldas se mova**.

Quanto à bateria, o suporte específico que eu comprei não gosta de ser colado, então eu simplesmente deixo balançando ou enfio dentro da luva.

Quando a luva estiver pronta, você pode construir a sua luva de protogen por cima dela, só deixe um buraco para o tecladinho do polegar. Você opera com um movimento de pinça entre o indicador e o polegar.
Como mencionado antes, dependendo do seu objetivo ou necessidade, você pode usar botões maiores, um botão por dedo, mover o tecladinho para o dorso da mão ou até usar como um relógio.