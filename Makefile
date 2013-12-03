all:
	g++ -lm -lGL -lGLU -lglut -o fileatom fileatom.cpp -Wall

linux:
	g++ -lm -lGL -lGLU -lglut -o fileatom fileatom.cpp -Wall

apple:
	g++ -frounding-math -framework GLUT -framework OpenGL -o fileatom fileatom.cpp -L "/System/Library/Frameworks/OpenGL.framework/Libraries" -lGL -lGLU -lm -ltcl -Wall

clean:
	rm -rf ./fileatom
