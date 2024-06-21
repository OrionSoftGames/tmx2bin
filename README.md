TMX2BIN v1.0 - by Orion_ [2008 - 2019]

https://orionsoft.games/
orionsoft@free.fr

Tiled is a very good Map Editor, unfortunately the file format output is an XML file, not easy to read on old system, so I made this tool to convert the xml into a binary file easily readable.

Tiled: http://www.mapeditor.org/

This version now support the tmx file format of Tiled 1.1.6
It will accept tmx files saved in plain XML or CSV or Base64 (uncompressed) format.

Usage: tmx2bin file.tmx [-bigendian | -byte | -layer N | -maxlayer N | -minusone | -noheader | -mergelayers]

Default output is little endian "unsigned short".
Add switch "-bigendian" to output big endian "unsigned short".
Add switch "-byte" to output "unsigned char".
Add switch "-layer N" to only process the layer N of the map. [0 -> ?]
Add switch "-maxlayer N" to only process the first N layers of the map. [1 -> ?]
Add switch "-minusone" to substract 1 to each tile index.
Add switch "-noheader" to remove the file header (only tile data)
Add switch "-mergelayers" to merge all layers into a single one (upper layers tiles replace bottom layers tiles)

Normal mode:      No tile =  0, First tile = 1
-minusone switch: No tile = -1, First tile = 0

The version 0.8 now support X/Y tile flip ! (using Megadrive compatible Flags)
X Flip Flag = 0x0800
Y Flip Flag = 0x1000

You can drag and drop a file on the exe and it should convert it.

The binary format output is as follow:

-----------------------------
| Info | Size | Description
-----------------------------
|  MAP |   3  | Signature, 3 bytes 'M','A','P'. Not affected by endian option.
|    n |   1  | Number of layers in the map. 1 byte.
|   xs |   2  | Width of the map. 1 word. Affected by endian option.
|   ys |   2  | Height of the map. 1 word. Affected by endian option.
| .... |   ?  | The rest of the file is each block id of the map. Size = xs * ys * number of layers.
		Each block id is 1 word (or byte). Affected by endian option.
