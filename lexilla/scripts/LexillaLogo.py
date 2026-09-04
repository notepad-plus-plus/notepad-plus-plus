# LexillaLogo.py
# Requires Python 3.6.
# Requires Pillow https://python-pillow.org/, tested with 7.2.0 on Windows 10

import random
from PIL import Image, ImageDraw, ImageFont

colours = [
(136,0,21,255),
(237,28,36,255),
(255,127,39,255),
(255,201,14,255),
(185,122,87,255),
(255,174,201,255),
(181,230,29,255),
(34,177,76,255),
(153,217,234,255),
(0,162,232,255),
(112,146,190,255),
(63,72,204,255),
(200,191,231,255),
]

width = 1280
height = 150

def drawLines(dr):
	for y in range(0,height, 2):
		x = 0
		while x < width:
			#lexeme = random.randint(2, 20)
			lexeme = int(random.expovariate(0.3))
			colour = random.choice(colours)
			strokeRectangle = (x, y, x+lexeme, y)
			dr.rectangle(strokeRectangle, fill=colour)
			x += lexeme + 3

def drawGuide(dr):
	for y in range(0,height, 2):
		x = 0
		while x < width:
			lexeme = int(random.expovariate(0.3))
			colour = (0x30, 0x30, 0x30)
			strokeRectangle = (x, y, x+lexeme, y)
			dr.rectangle(strokeRectangle, fill=colour)
			x += lexeme + 3

def drawLogo():
	# Ensure same image each time
	random.seed(1)

	# Georgia bold italic
	font = ImageFont.truetype(font="georgiaz.ttf", size=190)

	imageMask = Image.new("L", (width, height), color=(0xff))
	drMask = ImageDraw.Draw(imageMask)
	drMask.text((30, -29), "Lexilla", font=font, fill=(0))

	imageBack = Image.new("RGB", (width, height), color=(0,0,0))
	drBack = ImageDraw.Draw(imageBack)
	drawGuide(drBack)

	imageLines = Image.new("RGB", (width, height), color=(0,0,0))
	dr = ImageDraw.Draw(imageLines)
	drawLines(dr)

	imageOut = Image.composite(imageBack, imageLines, imageMask)

	imageOut.save("../doc/LexillaLogo.png", "png")

	imageDoubled = imageOut.resize((width*2, height * 2), Image.NEAREST)

	imageDoubled.save("../doc/LexillaLogo2x.png", "png")

drawLogo()
