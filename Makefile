all:
	g++ -lm -lGL -lGLU -lglut -o fileatom fileatom.cpp -Wall

clean:
	rm -rf ./fileatom
