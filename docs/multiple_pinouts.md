We used to think **Zigbee Manufacturer** (`_TZ3000_xxxxxxxx`) can reliably indicate a unique device, but AVATTO has proven us wrong.

There are some devices that have different pinouts, **even if their manufacturer names are identical**.  
Please trace the board pinout with a multimeter if your device is on the list below (especially if you bought it from a different source).

If your device has the **default pinout**, you can safely proceed with the update (main branch).  
If your device has a **different pinout**, please request a custom build on the issue page for the respective device.

Known cases:

<details>
<summary> <code>_TZ3000_5ajpkyq6</code> - AVATTO ZWSM16-4 </summary>
<br>

Pinout 1 (was default until v17.x), reported by @paddenstoeltje in [#9](https://github.com/romasku/tuya-zigbee-switch/issues/9):  
`BC2u;LD2i;SD7u;RB6;SC0u;RD4;SA0u;RC1;SA1u;RC4;`

**Pinout 2 (is default since v17.x)**, reported by @clumsy-stefan and @andrei-lazarov in [#9](https://github.com/romasku/tuya-zigbee-switch/issues/9):  
`BC2u;LD2i;SD3u;RC0;SD7u;RD4;SB6u;RC1;SA0u;RC4;`  
Source: [AliExpress](https://vi.aliexpress.com/item/1005007247647375.html) (no affiliate)

</details>


