# lanzaServidor.sh
# Lanza el servidor que es un daemon y varios clientes
# las ordenes est�n en un fichero que se pasa como tercer par�metro
./servidor
./cliente localhost TCP ordenes.txt &
./cliente localhost TCP ordenes1.txt &
./cliente localhost TCP ordenes2.txt &
./cliente localhost UDP ordenes2.txt &
./cliente localhost UDP ordenes2.txt &
./cliente localhost UDP ordenes2.txt &




























 
