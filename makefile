# make rule primaria con dummy target ‘all’--> non crea alcun file all ma fa un complete build
# che dipende dai target client e server scritti sotto
all: cli server td kd

# make rule per il client
client: cli.o
	gcc -Wall client.o -o client
#make rule del server
server: server.o
		gcc -Wall server.o -o server

td: td.o
	gcc -Wall td.o -o td
kd: kd.o
	gcc -Wall kd.o -o kd
# pulizia dei file della compilazione (eseguito con ‘make clean’ da terminale)
clean:
	rm *o cli server td kd