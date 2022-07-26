#include "funcionesMemoria.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>

//Antes de esto debo mandarle a cpu tam_Pagina y cant_entradas_por_pagina
int pidActual;//Viene de Kernel, para devolver nroDeTabla1erNivel
//int entradaDeTablaDe1erNivel = 2;  Viene de cpu(calculado con la dir logica), con esto devuelvo nroTablaDe2doNivel
int server_fd;
int contPaginas = 0;
int contTablas2doNivel = 0;
int flagDeEntradasPorTabla = 0;
int contadorDeMarcosPorProceso = 0;

int posicionDelPuntero;

int contadorDeEntradasPorProceso = 0;

char* pathDeArchivos;

pthread_mutex_t mutexMemoriaData;

t_list* listaT1nivel;
t_list* listaDeMarcos;
t_list* listaT2Nivel;
t_list* listaDePaginasEnMemoria;


int main(void) {
	pathDeArchivos = string_new();
	pathDeArchivos = "/home/utnso/swap";

	logger = log_create("memoria.log", "MEMORIA", 1, LOG_LEVEL_INFO);
	crearConfiguraciones();
	crearDirectorio();

	listaT1nivel = list_create();
	listaDeMarcos = list_create();
	listaT2Nivel = list_create();
	listaDePaginasEnMemoria = list_create();
	inicializarMarcos();
	inicializarEstructuras(pidActual);//esto tiene que estar despues de recibir el pid de kernel
	//conexionConKernel();
	//liberar_conexion(clienteKernel);
	conexionConCpu();

	log_info(logger,"Fin de memoria");
}

void crearDirectorio(){

	errno = 0;
	int ret = mkdir(pathDeArchivos,S_IRWXU);
	if (ret == -1) {
		switch (errno) {
			case EACCES :
				log_info(logger,"No permite escribir");
				break;
			case EEXIST:
				log_info(logger,"Ya existe la direccion de swap");
				break;
			}
	}
}


int conexionConCpu(void){

	int server_fd = iniciar_servidor(); // tendria que estar comentado porque viene despues de coenxion con kernel
	log_info(logger, "Memoria lista para recibir a Cpu");
	clienteCpu = esperar_cliente(server_fd);

	t_list* listaQueContieneNroTabla1erNivelYentrada = list_create();
	t_list* listaQueContieneEntradaDeTabla2doNivel = list_create();
	t_list* listaQueContieneDireccionFisca = list_create();
	t_list* listaQueContieneValorAEscribir = list_create();
	t_list* listaQueContieneDirFisica1YDirFisica2 = list_create();

	//int a = 1;
	while(1) {
		int cod_op = recibir_operacion(clienteCpu);
		pthread_mutex_lock(&mutexMemoriaData);
		
		sleep(retardoMemoria/1000); //lo que se tarda en acceder a memoria
		//log_info(logger,"Accediendo a memoria espere %d segundos\n",retardoMemoria/1000);
		
		int nroTabla2doNivel;
		switch (cod_op) {
			case MENSAJE_CPU_MEMORIA://mensaje de pedido tam pag y cant entradas
				recibir_mensaje(clienteCpu);//recibe el pedido de tam_pag y cant_entradas
				enviarTamanioDePaginaYCantidadDeEntradas(clienteCpu);
				break;
			case PRIMER_ACCESO://envio de nro tabla 2do nivel
				listaQueContieneNroTabla1erNivelYentrada = recibir_paquete_int(clienteCpu); // lista con valores de nro y entrada de tabla

				nroTabla2doNivel = leerYRetornarNroTabla2doNivel(listaQueContieneNroTabla1erNivelYentrada);

				enviarNroTabla2doNivel(clienteCpu,nroTabla2doNivel);
				break;

			case SEGUNDO_ACCESO://poner cases mas expresivos envio de marco
				listaQueContieneEntradaDeTabla2doNivel = recibir_paquete_int(clienteCpu);
				int entradaTabla2doNivel = (int) list_get(listaQueContieneEntradaDeTabla2doNivel,0);
				log_info(logger,"Me llego  la entrada de segundo nivel %d",entradaTabla2doNivel);

				int marco =  marcoSegunIndice(nroTabla2doNivel,entradaTabla2doNivel);

				enviarMarco(clienteCpu,marco);

				break;
			case READ://caso: me envia dir fisica y leo el valor de esa direccion
				listaQueContieneDireccionFisca = recibir_paquete_int(clienteCpu);

				marco = (int) list_get(listaQueContieneDireccionFisca,0); // por ahora piso la variable de arriba despues ver como manejar el tema de marco que envio y marco que recibo
				int desplazamiento = (int) list_get(listaQueContieneDireccionFisca,1);
				int leer = (int) list_get(listaQueContieneDireccionFisca,2);

				log_info(logger,"Me llego el marco %d con desplazamiento %d",marco,desplazamiento);

				if(leer == 1){
					log_info(logger,"-------------------READ-------------------");

					uint32_t numeroALeer = leerElPedido(marco,desplazamiento);
					log_info(logger, "Valor leido: %u",numeroALeer);

					log_info(logger,"-------------------READ-------------------\n");
				}

				break;
			case WRITE://caso: me envia dir fisica y escribo el valor en esa direccion
				listaQueContieneValorAEscribir = recibir_paquete_int(clienteCpu);
				uint32_t valorAEscribir = (uint32_t) list_get(listaQueContieneValorAEscribir,0);

				log_info(logger,"-------------------WRITE-------------------");

				log_info(logger,"Me llego el valor a escribir: %u",valorAEscribir);
				escribirElPedido((uint32_t) valorAEscribir,marco,desplazamiento); //casteo para que no joda el warning

				log_info(logger,"-------------------WRITE-------------------\n");
				break;
			case COPY://caso copiar
				listaQueContieneDirFisica1YDirFisica2 = recibir_paquete_int(clienteCpu);

				int marcoDeDestino = (int) list_get(listaQueContieneDirFisica1YDirFisica2,0);
				int desplazamientoDestino = (int) list_get(listaQueContieneDirFisica1YDirFisica2,1);
				int marcoDeOrigen = (int) list_get(listaQueContieneDirFisica1YDirFisica2,2);
				int desplazamientoOrigen = (int) list_get(listaQueContieneDirFisica1YDirFisica2,3);

				log_info(logger,"-------------------COPIAR-------------------");

				copiar(marcoDeDestino,desplazamientoDestino,marcoDeOrigen,desplazamientoOrigen);
				uint32_t datoALeer = leerElPedido(marcoDeDestino,desplazamientoDestino);
				log_info(logger,"Se copio el valor %u en la dir fisica:(marco %d offset %d)",datoALeer,marcoDeDestino,desplazamientoDestino);

				log_info(logger,"-------------------COPIAR-------------------\n");
				break;
//			case SUSPENSION:
//
//				recibir_mensaje(clienteCpu);
//				suspensionDeProceso(pid);
//
//				break;
//			case DESUSPENSION:
//
//				recibir_mensaje(clienteCpu);
				break;
			case -1:
				log_error(logger, "Se desconecto el cliente. Terminando conexion");
				return EXIT_FAILURE;
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				break;
		}

		 pthread_mutex_unlock(&mutexMemoriaData);
	}
	return EXIT_SUCCESS;
}

void crearConfiguraciones(){
	t_config* config = config_create("memoria.config");

	if(config  == NULL){
		printf("Error leyendo archivo de configuración. \n");
	}

	tamanioDeMemoria = config_get_int_value(config,"TAM_MEMORIA");
	tamanioDePagina = config_get_int_value(config,"TAM_PAGINA");

	entradasPorTabla = config_get_int_value(config,"ENTRADAS_POR_TABLA");
	retardoMemoria = config_get_int_value(config,"RETARDO_MEMORIA");
	algoritmoDeReemplazo = config_get_string_value(config,"ALGORITMO_REEMPLAZO");
	marcosPorProceso = config_get_int_value(config,"MARCOS_POR_PROCESO");
	retardoSwap = config_get_int_value(config,"RETARDO_SWAP");

}

void inicializarEstructuras(int pid){
	memoria = malloc(tamanioDeMemoria); // inicializo el espacio de usuario en memoria

	t_primerNivel* tablaPrimerNivel = malloc(sizeof(t_primerNivel));
	tablaPrimerNivel->pid = pid;
	tablaPrimerNivel->tablasDeSegundoNivel = list_create();

	//For tablas 2do nivel
	for(int i=0;i<entradasPorTabla;i++){

		t_segundoNivel* tablaDeSegundoNivel = malloc(sizeof(t_segundoNivel));
		tablaDeSegundoNivel->entradas = list_create();
		tablaDeSegundoNivel->numeroTabla = contTablas2doNivel;
		contTablas2doNivel++; //Contador global que asigna el numero de tabla de 2do nivel a nivel global, igual al indice en la lista

		cargarEntradasDeTabla2doNivel(tablaDeSegundoNivel);
		//tablaDeSegundoNivel->marcos = contadorDeMarcos;


		list_add(tablaPrimerNivel->tablasDeSegundoNivel, tablaDeSegundoNivel);
		list_add(listaT2Nivel,tablaDeSegundoNivel);


		//free(tablaDeSegundoNivel);
	}
	contadorDeEntradasPorProceso = 0;


	list_add(listaT1nivel,tablaPrimerNivel);

	//free(tablaPrimerNivel);

	crearSwap(pid);
}

void inicializarMarcos(){

	for(int j=0;j<(tamanioDeMemoria/tamanioDePagina);j++){
		marco* unMarco = malloc(sizeof(marco));
		unMarco->numeroDeMarco = j;
		unMarco->marcoLibre = 0;
		list_add(listaDeMarcos,unMarco);

		//free(unMarco);

	}
}

void cargarEntradasDeTabla2doNivel(t_segundoNivel* tablaDeSegundoNivel){

	for(int j=0;j<entradasPorTabla;j++){ //crear x cantidad de marcos y agregar de a 1 a la lista
			entradaTabla2doNivel* unaEntradaDeTabla2doNivel = malloc(sizeof(entradaTabla2doNivel));

			unaEntradaDeTabla2doNivel->numeroDeEntradaPorProceso = contadorDeEntradasPorProceso;
			unaEntradaDeTabla2doNivel->presencia = 0;
			unaEntradaDeTabla2doNivel->uso = 0;
			unaEntradaDeTabla2doNivel->modificado = 0;
			contadorDeEntradasPorProceso++;

			list_add(tablaDeSegundoNivel->entradas,unaEntradaDeTabla2doNivel);

		}
}

void crearSwap(int pid){

	char* nombrePathCompleto = nombreArchivoProceso(pid);

		FILE* archivoSwap = fopen(nombrePathCompleto, "w+");

		for(int i=0; i<(entradasPorTabla*entradasPorTabla);i++){

			for(int i=0; i<(entradasPorTabla*entradasPorTabla);i++){
				fwrite("0000",sizeof(int),1,archivoSwap);
			}

			fputs ("\n", archivoSwap);
		}

		fclose(archivoSwap);

}

char* nombreArchivoProceso(int pid){
	char* nombrePathCompleto = string_new();
	char* nombrePath = string_new();
	char* nombreArchivo = string_new();

	char* pidString = string_itoa(pid);

	nombrePath ="/home/utnso/swap/";

	nombreArchivo = pidString;

	string_append(&nombreArchivo,".swap");
	//string_append(&nombrePathCompleto,nombreArchivo);

	string_append(&nombrePathCompleto,nombrePath);
	string_append(&nombrePathCompleto,nombreArchivo);
	return nombrePathCompleto;
}

entradaTabla2doNivel* entradaCargadaConMarcoAsignado(int nroDemarco){
	entradaTabla2doNivel* unaEntrada = malloc(sizeof(entradaTabla2doNivel));
	for(int i = 0; i<list_size(listaDePaginasEnMemoria);i++){
		unaEntrada = list_get(listaDePaginasEnMemoria,i);

		if(unaEntrada->numeroMarco == nroDemarco){
			return unaEntrada;
		}
	}
}

void escribirElPedido(uint32_t datoAEscribir,int marco,int desplazamiento){
	int posicionDeDatoAEscribir = marco * tamanioDePagina + sizeof(uint32_t)*desplazamiento;// se repite siempre
	memcpy(&memoria+posicionDeDatoAEscribir, &datoAEscribir, sizeof(uint32_t)); //casteo para que no joda el warning

	entradaTabla2doNivel* entradaAEscribir = entradaCargadaConMarcoAsignado(marco);
	entradaAEscribir->modificado = 1;
	enviar_mensaje("Se escribio el valor correctamente",clienteCpu);
}

uint32_t leerElPedido(int marco,int desplazamiento){
	uint32_t datoALeer;
	int posicion = marco * tamanioDePagina + sizeof(uint32_t)*desplazamiento;
	memcpy(&datoALeer,&memoria+posicion,sizeof(uint32_t));

	if(datoALeer != 0){
		return datoALeer;
	}
}

void copiar(int marcoDeDestino,int desplazamientoDestino,int marcoDeOrigen,int desplazamientoOrigen){
	uint32_t datoACopiar;
	int posicionACopiar = marcoDeDestino * tamanioDePagina + sizeof(uint32_t)*desplazamientoDestino;
	int posicionDondeSeCopia = marcoDeOrigen * tamanioDePagina + sizeof(uint32_t)*desplazamientoOrigen;

	memcpy(&datoACopiar,&memoria+posicionDondeSeCopia,sizeof(uint32_t));
	memcpy(&memoria+posicionACopiar,&datoACopiar,sizeof(uint32_t));
}

void liberarEspacioEnMemoria(t_primerNivel* unaTablaDe1erNivel){
	t_segundoNivel* tablaDe2doNivel = malloc(sizeof(t_segundoNivel));
	entradaTabla2doNivel* unaEntrada = malloc(sizeof(entradaTabla2doNivel));
	marco* marcoAsignado = malloc(sizeof(marco));

	for(int i = 0; i<entradasPorTabla;i++){
		tablaDe2doNivel = list_get(unaTablaDe1erNivel->tablasDeSegundoNivel,i);

		for(int j = 0;j<entradasPorTabla;j++){
			unaEntrada = list_get(tablaDe2doNivel->entradas,j);//acaa
			if(unaEntrada->presencia == 1 && unaEntrada->modificado ==0){
				sacarMarcoAPagina(unaEntrada);
				marcoAsignado = buscarMarco(unaEntrada->numeroMarco);
				marcoAsignado->marcoLibre=0;

			}
			if(unaEntrada->presencia == 1 && unaEntrada->modificado ==1){
				escribirEnSwap(unaEntrada->numeroMarco,unaTablaDe1erNivel->pid,unaEntrada->numeroDeEntradaPorProceso);
				sacarMarcoAPagina(unaEntrada);
				marcoAsignado = buscarMarco(unaEntrada->numeroMarco);
				marcoAsignado->marcoLibre=0;
			}
		}

	}

list_clean(listaDePaginasEnMemoria);
}


void escribirEnSwap(int numeroDeMarco,int pid,int numeroDePagina){

	char* nombreDelArchivo = nombreArchivoProceso(pid);
	FILE* archivoSwap = fopen(nombreDelArchivo, "r+");

	int posicionDeLaPaginaEnSwapInicial = (entradasPorTabla*(tamanioDePagina/sizeof(int))*numeroDePagina) + numeroDePagina;

	for(int i=0; i<(tamanioDePagina/sizeof(uint32_t));i++){
	fseek(archivoSwap, posicionDeLaPaginaEnSwapInicial, SEEK_SET);
	//int posicionDeValorEnMemoria = numeroDeMarco*tamanioDePagina;
	fseek(archivoSwap, i*4, SEEK_CUR);

	int posicionDeValorEnMemoria = numeroDeMarco*tamanioDePagina + i*sizeof(uint32_t);
	uint32_t datoAEscribir = leerElPedido(numeroDeMarco,posicionDeValorEnMemoria);//Aca le agregue el numero de marco como primer parametro, revisar si esta bien
	char* datoAEscribirEnChar = string_itoa((uint32_t) datoAEscribir);
	fputs(datoAEscribirEnChar,archivoSwap);

	}

	fclose(archivoSwap);
}

void leerDeSwap(int numeroDePagina,int numeroDeMarco){
	char* parteDePagina = string_new();

	char* nombreDelArchivo = nombreArchivoProceso(pidActual); // capaz hay que pasarselo
	FILE* archivoSwap = fopen(nombreDelArchivo, "r");

	int posicionDeLaPaginaALeer = (entradasPorTabla*(tamanioDePagina/sizeof(int))*numeroDePagina) + numeroDePagina;


	for(int i = 0; i<entradasPorTabla*entradasPorTabla;i++){
	fseek(archivoSwap, posicionDeLaPaginaALeer, SEEK_SET);
	fseek(archivoSwap, i*4, SEEK_CUR);

	fgets(parteDePagina,sizeof(uint32_t)+1,archivoSwap);

	uint32_t parteDePaginaEnInt = atoi(parteDePagina);

	memcpy(&memoria+(tamanioDePagina*numeroDeMarco)+i*sizeof(uint32_t),&parteDePaginaEnInt, sizeof(uint32_t));
//	log_info(logger,parteDePagina);

	}
}

void suspensionDeProceso(int pid){
	int nroTabla1ernivel = buscarNroTablaDe1erNivel(pid);
	t_primerNivel* tablaDe1erNivel = malloc(sizeof(t_primerNivel));
	tablaDe1erNivel = list_get(listaT1nivel,nroTabla1ernivel);
	liberarEspacioEnMemoria(tablaDe1erNivel);

}

void finalizacionDeProceso(int pid){
	char* nombreDelArchivo = nombreArchivoProceso(pid);
	int nroTabla1ernivel = buscarNroTablaDe1erNivel(pid);
	t_primerNivel* tablaDe1erNivel = malloc(sizeof(t_primerNivel));
	tablaDe1erNivel = list_get(listaT1nivel,nroTabla1ernivel);
	liberarEspacioEnMemoria(tablaDe1erNivel);

	remove(nombreDelArchivo);
}


int buscarNroTablaDe1erNivel(int pid){
	t_primerNivel* unaTablaDe1erNivel = malloc(sizeof(t_primerNivel));

	for(int i=0;i < list_size(listaT1nivel);i++){

		unaTablaDe1erNivel = list_get(listaT1nivel,i);

		if(unaTablaDe1erNivel->pid == pid){
			return i;
		}
	}
	return 4;//Puse 4 para que no me confunda si retorno 0
	free(unaTablaDe1erNivel);
}

//Funcion para obtener el primer marco libre
marco* siguienteMarcoLibre(){
	marco* unMarco = malloc(sizeof(marco));

	for(int i=0;i < list_size(listaDeMarcos);i++){

		unMarco = list_get(listaDeMarcos,i);

		if(unMarco->marcoLibre == 0){
			return unMarco;
		}
	}
}


void reemplazarTodosLosUsoACero(t_list* listaDeEntradasDe2doNivel){
	entradaTabla2doNivel* unaEntrada = malloc(sizeof(entradaTabla2doNivel));

	for(int i = 0; i<list_size(listaDeEntradasDe2doNivel);i++){
		unaEntrada = list_get(listaDeEntradasDe2doNivel,i);
		unaEntrada->uso = 0;
			}
}

int posicionDePunteroDelAlgoritmo(int i){
	posicionDelPuntero += i;

	if(posicionDelPuntero<marcosPorProceso){
		return posicionDelPuntero;
	}else{
		posicionDelPuntero = 0;
		return	posicionDelPuntero;
	}
}

int algoritmoClock(t_list* listaDeEntradasDe2doNivel){
	entradaTabla2doNivel* unaEntrada = malloc(sizeof(entradaTabla2doNivel));

	for(int i = 0; i<list_size(listaDeEntradasDe2doNivel);i++){
		if(i==0){
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(0);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}else{
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}

		unaEntrada = list_get(listaDeEntradasDe2doNivel,posicionDelPuntero);

		if(unaEntrada->uso == 0){
			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);//Si reemplaza se mueve al siguiente
			log_info(logger,"PF, la posicion del puntero en la lista de marcos queda en: %d", posicionDelPuntero);
			return numeroDeMarcoAReemplazar;
		}
	}
	log_info(logger,"No hay ninguna pagina con bit de uso en 0");
	log_info(logger,"Por algoritmo reemplazo todos los bit de uso a 0 y busco de nuevo");
	posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
	reemplazarTodosLosUsoACero(listaDeEntradasDe2doNivel);

	for(int i = 0; i<list_size(listaDeEntradasDe2doNivel);i++){
		if(i==0){
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(0);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}else{
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}

		unaEntrada = list_get(listaDeEntradasDe2doNivel,posicionDelPuntero);

		if(unaEntrada->uso == 0){
			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);//Si reemplaza se mueve al siguiente
			log_info(logger,"PF, la posicion del puntero en la lista de marcos queda en: %d", posicionDelPuntero);
			return numeroDeMarcoAReemplazar;
		}
	}

}
// delegar funciones del if y hacer funcion de retorno

int algoritmoClockM (t_list* listaDeEntradasDe2doNivel){
	entradaTabla2doNivel* unaEntrada = malloc(sizeof(entradaTabla2doNivel));

	for(int i = 0; i<list_size(listaDeEntradasDe2doNivel);i++){
		if(i==0){
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(0);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}else{
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}

		unaEntrada = list_get(listaDeEntradasDe2doNivel,posicionDelPuntero);

		if(unaEntrada->uso == 0 && unaEntrada->modificado == 0){
			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);//Si reemplaza se mueve al siguiente
			log_info(logger,"PF, la posicion del puntero en la lista de marcos queda en: %d", posicionDelPuntero);
			return numeroDeMarcoAReemplazar;
		}
	}

	log_info(logger,"No hay ninguna pagina con bit de uso y modificado en 0");
	log_info(logger,"Busco con bit de uso en 0 y modificado en 1");

	for(int i = 0; i<list_size(listaDeEntradasDe2doNivel);i++){
		if(i==0){
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(0);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}else{
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}

		unaEntrada = list_get(listaDeEntradasDe2doNivel,posicionDelPuntero);

		if(unaEntrada->uso == 0 && unaEntrada->modificado == 1){
			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);//Si reemplaza se mueve al siguiente
			log_info(logger,"PF, la posicion del puntero en la lista de marcos queda en: %d", posicionDelPuntero);
			return numeroDeMarcoAReemplazar;
		}
	}

	log_info(logger,"No hay ninguna pagina con bit de uso en 0 y modificado en 1");
	log_info(logger,"Reemplazo todos los bit de uso en 0");
	reemplazarTodosLosUsoACero(listaDeEntradasDe2doNivel);
	log_info(logger,"Busco con bit de uso en 0 y modificado en 0 ");

	for(int i = 0; i<list_size(listaDeEntradasDe2doNivel);i++){
		if(i==0){
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(0);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}else{
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}

		unaEntrada = list_get(listaDeEntradasDe2doNivel,posicionDelPuntero);

		if(unaEntrada->uso == 0 && unaEntrada->modificado == 0){
			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);//Si reemplaza se mueve al siguiente
			log_info(logger,"PF, la posicion del puntero en la lista de marcos queda en: %d", posicionDelPuntero);
			return numeroDeMarcoAReemplazar;
		}
	}
	log_info(logger,"No hay ninguna pagina con bit de uso en 0 y modificado en 0");
	log_info(logger,"Busco con bit de uso en 0 y modificado en 1");

	for(int i = 0; i<list_size(listaDeEntradasDe2doNivel);i++){
		if(i==0){
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(0);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}else{
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);
			log_info(logger,"La posicion del puntero en la lista de marcos es: %d", posicionDelPuntero);
		}

		unaEntrada = list_get(listaDeEntradasDe2doNivel,posicionDelPuntero);

		if(unaEntrada->uso == 0 && unaEntrada->modificado == 1){
			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			posicionDelPuntero = posicionDePunteroDelAlgoritmo(1);//Si reemplaza se mueve al siguiente
			log_info(logger,"PF, la posicion del puntero en la lista de marcos queda en: %d", posicionDelPuntero);
			return numeroDeMarcoAReemplazar;
		}
	}
}

marco* buscarMarco(int nroDeMarco){
	marco* unMarco = malloc(sizeof(marco));

	for(int i=0;i < list_size(listaDeMarcos);i++){

		unMarco = list_get(listaDeMarcos,i);

		if(unMarco->numeroDeMarco == nroDeMarco){
			return unMarco;
		}
	}
	free(unMarco);
}

//Funcion de cargar una pagina, por ahora la hago global y en 1 proceso porque no se como funciona
void cargarPagina(entradaTabla2doNivel* unaEntrada){
	marco* marcoAAsignar;
	//Caso en el que se puede asignar un marco a un proceso de manera libre
	if(contadorDeMarcosPorProceso<marcosPorProceso){

		marcoAAsignar = siguienteMarcoLibre();
		modificarPaginaACargar(unaEntrada,marcoAAsignar->numeroDeMarco);
		list_add(listaDePaginasEnMemoria,unaEntrada);
		contadorDeMarcosPorProceso++; // ANALIZAR CONTADOR
		if(unaEntrada->modificado == 1){
					leerDeSwap(unaEntrada->numeroDeEntradaPorProceso,marcoAAsignar->numeroDeMarco);
				}
	}else{//Caso en el que ya el proceso tiene maxima cantidad de marcos por proceso y hay que desalojar 1
		if(strcmp(algoritmoDeReemplazo,"CLOCK") == 0){//PARA ESTOS CASOS NO ES LIST_ADD TENGO QUE REEMPLAZAR LA ENTRADA QUE SACO
			int marcoAAsignar = algoritmoClock(listaDePaginasEnMemoria);
			modificarPaginaACargar(unaEntrada,marcoAAsignar);
			int posicionAReemplazar = indiceDeEntradaAReemplazar(marcoAAsignar);
			list_replace(listaDePaginasEnMemoria, posicionAReemplazar, unaEntrada);
			if(unaEntrada->modificado == 1){
						leerDeSwap(unaEntrada->numeroDeEntradaPorProceso,marcoAAsignar);
					}

		}else if(strcmp(algoritmoDeReemplazo,"CLOCK M") == 0){
			int marcoAAsignar = algoritmoClockM(listaDePaginasEnMemoria);
			modificarPaginaACargar(unaEntrada,marcoAAsignar);
			int posicionAReemplazar = indiceDeEntradaAReemplazar(marcoAAsignar);
			list_replace(listaDePaginasEnMemoria, posicionAReemplazar, unaEntrada);
			if(unaEntrada->modificado == 1){
						leerDeSwap(unaEntrada->numeroDeEntradaPorProceso,marcoAAsignar);
					}
		}
	}
}

int indiceDeEntradaAReemplazar(int numeroDeMarco){
	entradaTabla2doNivel* unaEntrada = malloc(sizeof(entradaTabla2doNivel));
	for(int i=0;i < list_size(listaDePaginasEnMemoria);i++){
		unaEntrada=list_get(listaDePaginasEnMemoria,i);
		if(unaEntrada->numeroMarco == numeroDeMarco){
			return i;
		}
	}
}

void modificarPaginaACargar(entradaTabla2doNivel* unaEntrada, int nroDeMarcoAASignar){
	unaEntrada->numeroMarco = nroDeMarcoAASignar;
	unaEntrada->presencia = 1;
	unaEntrada->uso = 1;
	//unaPagina->modificado = 0; ANALIZAR CASO DE MODIFICADO EN 1
	marco* marcoAsignado = buscarMarco(nroDeMarcoAASignar);
	marcoAsignado->marcoLibre = 1;
}

void sacarMarcoAPagina(entradaTabla2doNivel* unaEntrada){
	unaEntrada->presencia = 0;
	unaEntrada->uso = 0;
	//unaEntrada->numeroMarco = -1;

}

void chequeoDeIndice(int indice){
	if(indice<entradasPorTabla){
		flagDeEntradasPorTabla = 1;
	}
}


int buscarNroTablaDe2doNivel(int numeroDeTabla2doNivel){
	t_segundoNivel* unaTablaDe2doNivel = malloc(sizeof(t_segundoNivel));

	for(int i=0;i < list_size(listaT2Nivel);i++){

		unaTablaDe2doNivel = list_get(listaT2Nivel,i);

		if(unaTablaDe2doNivel->numeroTabla == numeroDeTabla2doNivel){
			return i;
		}
	}
	// return 4;//Puse 4 para que no me confunda si retorno 0
	free(unaTablaDe2doNivel);
}

int leerYRetornarNroTabla2doNivel(t_list* nroTabla1erNivelYentrada){
	nroTablaDePaginas1erNivel = (int) list_get(nroTabla1erNivelYentrada,0);
	entradaTablaDePaginas1erNivel = (int) list_get(nroTabla1erNivelYentrada,1);

	log_info(logger,"Me llego el nroTabla1erNivel %d y con la entrada %d",nroTablaDePaginas1erNivel,entradaTablaDePaginas1erNivel);

	return numeroTabla2doNivelSegunIndice(nroTablaDePaginas1erNivel,entradaTablaDePaginas1erNivel);
}


int numeroTabla2doNivelSegunIndice(int numeroTabla1erNivel,int indiceDeEntrada){
	chequeoDeIndice(indiceDeEntrada);

	if(flagDeEntradasPorTabla == 1){
		t_primerNivel* unaTablaDe1erNivel = malloc(sizeof(t_primerNivel));
		t_segundoNivel* unaTablaDe2doNivel = malloc(sizeof(t_segundoNivel));
		unaTablaDe1erNivel = list_get(listaT1nivel,numeroTabla1erNivel);
		unaTablaDe2doNivel = list_get(unaTablaDe1erNivel->tablasDeSegundoNivel,indiceDeEntrada);
		flagDeEntradasPorTabla = 0;

		//free(unaTablaDe1erNivel);
		return unaTablaDe2doNivel->numeroTabla;
	}else{
		return -1;//Si da -1 printeo un error
	}

}

int marcoSegunIndice(int numeroTabla2doNivel,int indiceDeEntradaTabla2doNivel){

	entradaTabla2doNivel* unaEntradaTabla2doNivel = malloc(sizeof(entradaTabla2doNivel));

	t_segundoNivel* unaTablaDe2doNivel = malloc(sizeof(t_segundoNivel));

	chequeoDeIndice(indiceDeEntradaTabla2doNivel);

	if(flagDeEntradasPorTabla == 1){
		int posicionDeLaTablaBuscada = buscarNroTablaDe2doNivel(numeroTabla2doNivel);
		unaTablaDe2doNivel = list_get(listaT2Nivel,posicionDeLaTablaBuscada);

		unaEntradaTabla2doNivel = list_get(unaTablaDe2doNivel->entradas,indiceDeEntradaTabla2doNivel);// ya con esto puedo recuperar el marco

		flagDeEntradasPorTabla = 0;

		if(unaEntradaTabla2doNivel->presencia == 1){
			return unaEntradaTabla2doNivel->numeroMarco;
		}else{
			cargarPagina(unaEntradaTabla2doNivel);
			return unaEntradaTabla2doNivel->numeroMarco;
	//ver tema de si es una pagina con info para swap
		}
	}

}

int conexionConKernel(void){

	server_fd = iniciar_servidor();
	log_info(logger, "Memoria lista para recibir a Kernel");
	clienteKernel = esperar_cliente(server_fd);

	t_list* listaQueContienePID = list_create();

	while(1) {
		int cod_op = recibir_operacion(clienteKernel);

		switch (cod_op) {

			case NRO_TP1:
				listaQueContienePID = recibir_paquete_int(clienteKernel);
				pidActual = (int) list_get(listaQueContienePID,0);

				inicializarEstructuras(pidActual);//inicializo estructuras
				inicializarMarcos();

				int nroTabla1erNivel = buscarNroTablaDe1erNivel(pidActual);

				enviarNroTabla1erNivel(clienteKernel,nroTabla1erNivel);

				break;
			case -1:
				log_error(logger, "Se desconecto el cliente. Terminando conexion");
				return EXIT_FAILURE;
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				break;
			}

	}

	return EXIT_SUCCESS;
}


void iterator(char* value) {
	log_info(logger,"%s", value);
}

// falta terminar programa


