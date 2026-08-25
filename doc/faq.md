# FAQ

**What is protopanda?**

Initially it was the name of the firmware I designed to make my life easier when making protogens. But now its a whole ecosystem including hardware, firmware and 3d models.

---

**Do I need to pay?**

Only if you wanna build it, then you'll have to pay for the components and parts hehe.

---

**Does it work on [insert MCU that is not Esp32s3]?**

No.

---

**Can it use MAXMAX7219 matrixes instead of HUB75**

Yes! [Check the configuration guide.](./configuration.md)

---

**Can it use a microphone?**

Yes! [Check the configuration guide.](./configuration.md)

---

**Do I really need the SD card?**

Yes. Technically its possible to adapt the code to use only internal flash, but that comes with a series of problems and storage space. So stick with the SD card.

---

**How do I change the side led pattern?**

You can [check the configuration guide.](./configuration.md) in the LED topic.

---

**I'm stuck at the "SD NOT FOUND" screen.**

Check your wiring. Seriously. Check again, each pin. Most of the times is just bad wiring.
If you soldered, check your solder. 
If it dont solve the problem, format the SD card to FAT32. 
If it still fails, try another SD card. 
If it still fails, replace the SD card module.

---

**Why boop is always triggered?**

You probally wired the boop sensor wrong. Swap the pins at `misc.json`:
```json
        "gpio": 48,
        "power_gpio": 13,
```
Swap 48 with 13 and 13 with 48.

---

**When I turn on my proto, i'm stuck in a "waiting controller" screen.**

First time protopanda boots, it will require a remote controller. You can skip this by changing some settings in `misc.json`.
You can disable bluetooth `""mode": "BLE",`. You can choose none or infrared. With none, no input will be used and protopanda will have no way to change expressions unless you code something.
In infrared mode, you'll need to add a infrared receiver to use a IR controller.

---

**I paired a controller already, but it still stuck on that controller screen!**

You need to connect the internal button! Its on the schematic. If you conntected it already its stuck pressed or you wired it wrong.

---

**I changed the facial expressions at the SD card but nothing changed.**

Expressions are pre decoded and cache to make displaying them faster. So you'll need to clear that cache.
Delete the 'cache' folder, or go to settings>Rebuild bulk file in your proto menu.
---

**I changed a .json file and reflashed the firmware, but nothing changed!**

JSON files should be at the SD card, not in the firmware. Change the json in your sd card and that will work.
---

**I got the reccomended controller from aliexpress, but its not pairing!**

Check if the controller is on. If is the same controller and if in `keybinds.json` this setting is set to true: `"enableHidControllers": true,`
