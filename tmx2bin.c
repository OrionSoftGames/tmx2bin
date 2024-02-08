/*
	TMX2BIN v1.0 by Orion_ [2010/2023]
	http://orionsoft.games/
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define	TILED_X_FLIP	0x80000000
#define	TILED_Y_FLIP	0x40000000
#define	TILE_X_FLIP		0x0800		// MD attributes
#define	TILE_Y_FLIP		0x1000

//------------------------------------------
// Base64 Stuff

unsigned char	Base64Table[256];
char			Base64TableInv[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void	InitBase64Table(void)
{
	int	i;

	memset(Base64Table, 0xFF, sizeof(Base64Table));
	for (i = 0; i < sizeof(Base64TableInv); i++)
		Base64Table[Base64TableInv[i]] = i;
}

int		Base64ToBin(unsigned char *str, int size)
{
	unsigned char	*bin = str, b64[4], step = 0;
	int				nsize = 0;
	char			c;

	while (size > 0)
	{
		if ((b64[step] = Base64Table[*str++]) == 0xFF)
			break;
		switch (step)
		{
			case 1:	*bin++ = (b64[0] << 2) | (b64[1] >> 4);	break;
			case 2:	*bin++ = (b64[1] << 4) | (b64[2] >> 2);	break;
			case 3:	*bin++ = (b64[2] << 6) |  b64[3];		break;
		}
		if (step)
			nsize++;
		size--;
		step = (step + 1) & 3;
	}

	return (nsize);
}


//------------------------------------------
// Endian Utils

void	WriteW(unsigned short word, FILE *f, int bige)
{
	if (bige)
		word = (word >> 8) | (word << 8);
	fwrite(&word, 2, 1, f);
}

void	WriteB(unsigned char byte, FILE *f)
{
	fwrite(&byte, 1, 1, f);
}

int	bige, MinusOne, bytemode, curlayer, MergeLayers;
unsigned short	*ptr, *map;

void	WriteTile(FILE *f, uint32_t data)
{
	unsigned short	d16 = data & 0x7FF;

	if (MinusOne)
		d16--;
	if (data & TILED_X_FLIP)
		d16 |= TILE_X_FLIP;
	if (data & TILED_Y_FLIP)
		d16 |= TILE_Y_FLIP;
	if (MergeLayers && curlayer && (!d16))	// avoid erasing previous layer tile in case of merging layers
	{
		ptr++;
		return;
	}
	*ptr++ = d16;
}

//------------------------------------------
// Main

int		main(int argc, char *argv[])
{
	InitBase64Table();

	printf("TMX2BIN v1.0 - by Orion_ [2010/2023] http://orionsoft.games/\n\n");

	if (argc > 1)
	{
		FILE *f = fopen(argv[1], "rb");

		unsigned char *buf, fname[256], *c, *cs, *p;
		int i, s, base64mode = 0, csvmode = 0, layer = -1, maxlayer = -1, NoHeader = 0;
		uint32_t *cl;

		MergeLayers = curlayer = bige = MinusOne = bytemode = 0;
		for (s = 2; s < argc; s++)
		{
			if (strcmp(argv[s], "-bigendian") == 0)
				bige = 1;
			else if (strcmp(argv[s], "-byte") == 0)
				bytemode = 1;
			else if (strcmp(argv[s], "-maxlayer") == 0)
			{
				s++;
				maxlayer = atoi(argv[s]);
			}
			else if (strcmp(argv[s], "-layer") == 0)
			{
				s++;
				layer = atoi(argv[s]);
			}
			else if (strcmp(argv[s], "-minusone") == 0)
				MinusOne = 1;
			else if (strcmp(argv[s], "-noheader") == 0)
				NoHeader = 1;
			else if (strcmp(argv[s], "-mergelayers") == 0)
				MergeLayers = 1;
		}

		if (f)
		{
			fseek(f, 0, SEEK_END);
			s = ftell(f);
			fseek(f, 0, SEEK_SET);
			buf = malloc(s);
			if (buf)
			{
				// Read the XML
				fread(buf, 1, s, f);
				fclose(f);
				strcpy(fname, argv[1]);
				strcpy(strchr(fname, '.'), ".map");

				f = fopen(fname, "wb");
				if (f)
				{
					unsigned char nlayer = 0, sign[3] = {'M','A','P'};
					unsigned short xs = 0, ys = 0;

					// Count the number of layers
					c = strstr(buf, "<layer");
					while (c)
					{
						c += 6;
						nlayer++;
						c = strstr(c, "<layer");
					}

					c = strstr(buf, "width=");
					cs = strstr(buf, "height=");

					// We have all the info
					if (c && cs && nlayer)
					{
						uint32_t	data;

						// Read Map Size
						c += 5 + 2;
						p = strchr(c, '"');
						*p = '\0';
						xs = atoi(c);
						*p = '"';

						cs += 6 + 2;
						p = strchr(cs, '"');
						*p = '\0';
						ys = atoi(cs);
						*p = '"';

						// Limit number of layer (if user asked for it)
						if (maxlayer > 0)
							if (nlayer > maxlayer)
								nlayer = maxlayer;

						// Our File Header
						if (!NoHeader)
						{
							fwrite(sign, 3, 1, f);
							if ((layer >= 0) || MergeLayers)
								fputc(1, f);	// Process only one layer
							else
								fputc(nlayer, f);
							WriteW(xs, f, bige);
							WriteW(ys, f, bige);
						}
						ptr = map = (unsigned short *)malloc(nlayer * xs * ys * sizeof(short));

						// Search each layer
						c = strstr(buf, "<data");
						if (strstr(c, "base64"))
							base64mode = 1;
						else
						{
							if (strstr(c, "csv"))
								csvmode = 1;
						}

						if (c)
							c = strchr(c, '>') + 1;
						while (c)
						{
							if (base64mode)
							{
								// Skip stuff
								while ((*c == ' ') || (*c == '\r') || (*c == '\n')) c++;
								cs = c;
								while ((*cs != ' ') && (*cs != '\r') && (*cs != '\n')) cs++;
								s = cs - c;
								s = Base64ToBin(c, s);
								if (!c)
									return (-1);

								if ((layer < 0) || (curlayer == layer))
								{
									s /= 4;
									cl = (uint32_t *)c;
									while (s--)
									{
										WriteTile(f, *cl);
										cl++;
									}
								}
							}
							else if (csvmode)	// New version of TMX
							{
								s = xs * ys;
								while (s--)
								{
									// Skip stuff
									while ((*c == ' ') || (*c == '\r') || (*c == '\n')) c++;
									// Search next delimiter
									cs = c;
									while ((*cs != ',') && (*cs != '\r') && (*cs != '\n')) cs++;
									*cs = '\0';
									data = atoi(c);
									c = cs + 1;
									if ((layer < 0) || (curlayer == layer))
										WriteTile(f, data);
								}
								cs = c;
							}
							else	// OLD version of TMX
							{
								cs = c;
								s = xs * ys;
								while (s--)
								{
									c = strstr(c, "<tile");
									if (c[5] == '/')
									{
										data = 0;
										c += 5;
									}
									else
									{
										c = strchr(c, '\"');
										c++;
										p = strchr(c, '\"');
										*p = 0;
										data = atoi(c);
										*p = '\"';
										c = p + 1;
									}
									if ((layer < 0) || (curlayer == layer))
										WriteTile(f, data);
								}
							}

							curlayer++;
							if (curlayer == nlayer)
								break;

							// Search next
							c = strstr(cs, "<data");
							if (c)
								c = strchr(c, '>') + 1;

							if (MergeLayers)	// Rewind Map Ptr if Layer Merging
								ptr = map;
						}

						// Write Final Data
						if ((layer >= 0) || MergeLayers)
							nlayer = 1;
						for (i = 0; i < nlayer * xs * ys; i++)
							if (bytemode)
								WriteB(map[i], f);
							else
								WriteW(map[i], f, bige);
						free(map);
					}
					else
						printf("Wrong file format !\n");
					fclose(f);
				}
				else
					printf("Can not create file '%s'\n", fname);
				free(buf);
			}
			else
				printf("Not enough memory\n");
		}
		else
			printf("Can not open file '%s'\n", argv[1]);
	}
	else
		printf("Usage: tmx2bin file.tmx [option(s)]\n\n"
				"Default output is little endian unsigned short\n"
				"Options: \'-bigendian\' to output big endian unsigned short.\n"
				"         \'-byte\' to output unsigned char.\n"
				"         \'-layer N\' to only process the layer N of the map. [0 -> ?]\n"
				"         \'-maxlayer N\' to only process the first N layers of the map. [1 -> ?]\n"
				"         \'-minusone\' to substract 1 to each tile index.\n"
				"         \'-noheader\' to remove the file header (only tile data)\n"
				"         \'-mergelayers\' to merge all layers into a single one\n"
				"                          (upper layers tiles replace bottom layers tiles)\n");
	return (0);
}
