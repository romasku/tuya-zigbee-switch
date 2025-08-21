# Debugging

[ZTU]: https://developer.tuya.com/en/docs/iot/ztu-module-datasheet?id=Ka45nl4ywgabp
[ZT2S]: https://developer.tuya.com/en/docs/iot/zt2s-module-datasheet?id=Kas9gdtath9p0
[ZT3L]: https://developer.tuya.com/en/docs/iot/zt3l-module-datasheet?id=Ka438n1j8nuvu
[3D_model]: https://www.printables.com/model/763948-tuya-cbu-flashing-jig


Currently, debugging is very rudimentary.  

It consits of adding **prints in the code** and building with the DEBUG flag enabled.  
(See [`make_scripts/make_debug_single.sh`](../make_scripts/make_debug_single.sh).)  

After you flash the debug build, the device will output the messages to the **UART TX pin**.  
The TX pin can be dedicated or shared with a GPIO pin.  
Check the diagrams: [ZTU], [ZT2S], [ZT3L].  

You can safely power and use the device with 3.3V DC from the programmer.  
It behaves exactly the same way it does when powered with 220V AC.

> [!WARNING]  
> Do not open the device while connected to mains!  
> Do not connect both mains voltage and 3.3V at the same time!  

Attach a probe to the TX pin of the switch and connect it to the RX pin of the receiver.  
(You no longer need the SWS and RST connections. These are only for flashing.)  

There are multiple options available for keeping the wires connected:  
soldering, [3D printed jig][3D_model], laser cut jig, PCBite kit, holding them with your hand.  
You might need to desolder some components to access the pins.

Use this command to find the port:

```bash
ls -l /dev/serial/by-id
```

Make sure the port is not already in use (stop the flasher) and  
start the serial monitor with this command (update the port):

```bash
sudo minicom -b 115200 -o -D /dev/ttyUSB0
```