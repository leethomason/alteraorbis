from PIL import Image, ImageDraw, ImageEnhance
import sys

size = 128, 128

inputIm = Image.open( sys.argv[1] )
samples = int( sys.argv[2] )
useColor = int( sys.argv[3] )

inputIm = inputIm.convert( "RGB" )
print inputIm
print( "Samples", samples )
palette = Image.new( "RGB", (256,256) );
print( palette );

inSize = inputIm.size[0]/samples, inputIm.size[1]
colors = []
offset = []

for i in range(samples+1):
	offset.append( 256*i/(samples+1) )
offset.append( 255 )

for i in range( samples ):
	pos = inSize[0]/2 + i*inSize[0], inSize[1]/2
	c = inputIm.getpixel( pos )
	print pos, c
	colors.append( c )

draw = ImageDraw.Draw( palette )

# Top and bottom bars.
for i in range( samples ):
	draw.rectangle(( offset[i+1], offset[0], offset[i+2], offset[1]), fill=colors[i] )
for i in range( samples ):
	draw.rectangle(( offset[0], offset[i+1], offset[1], offset[i+2]), fill=colors[i] )

# Mixes
for i in range( samples ):
	for j in range( samples ):
		cprime = (colors[i][0]+colors[j][0])/2, (colors[i][1]+colors[j][1])/2, (colors[i][2]+colors[j][2])/2 
		draw.rectangle(( offset[i+1], offset[j+1], offset[i+2], offset[j+2]), fill=cprime )

if useColor == 0:
	brightIm = ImageEnhance.Brightness( palette ).enhance( 1.3 );
	darkIm = ImageEnhance.Brightness( palette ).enhance( 0.7 );
else:
	brightIm = ImageEnhance.Color( palette ).enhance( 1.5 );
	darkIm = ImageEnhance.Color( palette ).enhance( 0.5 );

# Bright & dark
for i in range( samples+1 ):
	for j in range( samples+1 ):
		if ( j > i ):
			pos = (offset[i]+offset[i+1])/2, (offset[j]+offset[j+1])/2 
			cdark   = darkIm.getpixel( pos );
			cbright = brightIm.getpixel( pos );
			draw.rectangle( ( offset[i], offset[j], (offset[i]+offset[i+1])/2, offset[j+1] ), fill=cdark );
			# not sure why the -1 is there: haven't worked out the exact coordinatess
			draw.rectangle( ( (offset[i]+offset[i+1])/2, offset[j], offset[i+1]-1, offset[j+1] ), fill=cbright )
			
palette.save( "palette.png", "PNG" );
brightIm.save( "paletteBright.png", "PNG" );
darkIm.save( "paletteDark.png", "PNG" );
print( "Done" );

