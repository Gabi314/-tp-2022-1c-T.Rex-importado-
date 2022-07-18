#include "funcionesMemoria.h"

//Antes de esto debo mandarle a cpu tam_Pagina y cant_entradas_por_pagina
int pid = 0;//Viene de Kernel, para devolver nroDeTabla1erNivel
//int entradaDeTablaDe1erNivel = 2;  Viene de cpu(calculado con la dir logica), con esto devuelvo nroTablaDe2doNivel

int contPaginas = 0;
int contTablas2doNivel = 0;
int flagDeEntradasPorTabla = 0;
int contadorDeMarcosPorProceso = 0;

int contadorDeEntradasPorProceso = 0;

t_list* listaT1nivel;
t_list* listaDeMarcos;
t_list* listaT2Nivel;
t_list* listaDePaginasEnMemoria;


int main(void) {
	logger = log_create("memoria.log", "MEMORIA", 1, LOG_LEVEL_INFO);
	crearConfiguraciones();

	listaT1nivel = list_create();
	listaDeMarcos = list_create();
	listaT2Nivel = list_create();
	listaDePaginasEnMemoria = list_create();


	inicializarEstructuras();
	inicializarMarcos();
	conexionConCpu();

	uint32_t datoAEscribir;
	datoAEscribir = leerElPedido(0);

	log_info(logger,"leer: %u",datoAEscribir);

	//escribirEnSwap(0,0,0,0);

	log_info(logger,"Fin de memoria");
}

int conexionConCpu(void){

	int server_fd = iniciar_servidor();
	log_info(logger, "Memoria lista para recibir a Cpu o Kernel");
	clienteCpu = esperar_cliente(server_fd);

	t_list* listaQueContieneNroTabla1erNivelYentrada = list_create();
	t_list* listaQueContieneEntradaDeTabla2doNivel = list_create();
	t_list* listaQueContieneDireccionFiscaYValorAEscribir = list_create();

	int a = 1;
	while (a == 1) {
		int cod_op = recibir_operacion(clienteCpu);

		int nroTabla2doNivel;
		switch (cod_op) {
			case MENSAJE:
				recibir_mensaje(clienteCpu);//recibe el pedido de tam_pag y cant_entradas
				enviarTamanioDePaginaYCantidadDeEntradas(clienteCpu);
				break;
			case PAQUETE:
				listaQueContieneNroTabla1erNivelYentrada = recibir_paquete_int(clienteCpu); // lista con valores de nro y entrada de tabla

				nroTabla2doNivel = leerYRetornarNroTabla2doNivel(listaQueContieneNroTabla1erNivelYentrada);

				enviarNroTabla2doNivel(clienteCpu,nroTabla2doNivel);
				break;

			case PAQUETE2://poner cases mas expresivos
				listaQueContieneEntradaDeTabla2doNivel = recibir_paquete_int(clienteCpu);
				int entradaTabla2doNivel = (int) list_get(listaQueContieneEntradaDeTabla2doNivel,0);
				log_info(logger,"Me llego  la entrada de segundo nivel %d",entradaTabla2doNivel);

				int marco =  marcoSegunIndice(nroTabla2doNivel,entradaTabla2doNivel);

				enviarMarco(clienteCpu,marco);

				break;
			case PAQUETE3:
				listaQueContieneDireccionFiscaYValorAEscribir = recibir_paquete_int(clienteCpu);

				marco = (int) list_get(listaQueContieneDireccionFiscaYValorAEscribir,0); // por ahora piso la variable de arriba despues ver como manejar el tema de marco que envio y marco que recibo
				int desplazamiento = (int) list_get(listaQueContieneDireccionFiscaYValorAEscribir,1);
				uint32_t valorAEscribir = (uint32_t) list_get(listaQueContieneDireccionFiscaYValorAEscribir,2);

				log_info(logger,"Me llego el marco %d con desplazamiento %d y valor a escribir %u",marco,desplazamiento,valorAEscribir);

				int posicionDeDatoAEscribir = marco * tamanioDePagina + sizeof(uint32_t)*desplazamiento;
				escribirElPedido((uint32_t) &valorAEscribir,posicionDeDatoAEscribir); //casteo para que no joda el warning

				uint32_t numeroALeer = leerElPedido(posicionDeDatoAEscribir);
				log_info(logger, "Numero leido: %u",numeroALeer);
				a = 0;
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

void crearConfiguraciones(){
	t_config* config = config_create("memoria.config");

	tamanioDeMemoria = config_get_int_value(config,"TAM_MEMORIA");
	tamanioDePagina = config_get_int_value(config,"TAM_PAGINA");

	entradasPorTabla = config_get_int_value(config,"ENTRADAS_POR_TABLA");
	retardoMemoria = config_get_int_value(config,"RETARDO_MEMORIA");
	algoritmoDeReemplazo = config_get_string_value(config,"ALGORITMO_REEMPLAZO");
	marcosPorProceso = config_get_int_value(config,"MARCOS_POR_PROCESO");
	retardoSwap = config_get_int_value(config,"RETARDO_SWAP");

}

void inicializarEstructuras(){
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
			contadorDeEntradasPorProceso++;

			list_add(tablaDeSegundoNivel->entradas,unaEntradaDeTabla2doNivel);

		}
}

void crearSwap(int pid){

	char* nombreArchivo = string_new();

	char* pidString = string_itoa(pid);
	nombreArchivo = pidString;

	string_append(&nombreArchivo,".swap");

	FILE* archivoSwap = fopen(nombreArchivo, "w+");

	for(int i=0; i<(tamanioDePagina/sizeof(uint32_t))*entradasPorTabla*entradasPorTabla;i++){

		uint32_t* datoAEscribir = 0;
		char* datoAEscribirEnChar = string_itoa((uint32_t) datoAEscribir);
		string_append(&datoAEscribirEnChar,"\n");

		fputs (datoAEscribirEnChar, archivoSwap);
	}


	fclose(archivoSwap);

}

void escribirElPedido(uint32_t datoAEscribir,int posicionDeDatoAEscribir){
	memcpy(&memoria+posicionDeDatoAEscribir,(uint32_t*) datoAEscribir, sizeof(uint32_t)); //casteo para que no joda el warning

	enviar_mensaje("Se escribio el valor correctamente",clienteCpu);
}

uint32_t leerElPedido(int posicion){
	uint32_t datoALeer;
	memcpy(&datoALeer,&memoria+posicion,sizeof(uint32_t));

	if(datoALeer != 0){
		return datoALeer;
	}
}

void copiar(int posicionDondeSeCopia,int posicionACopiar){
	uint32_t* datoACopiar = malloc(sizeof(uint32_t));
	memcpy(&datoACopiar,&memoria+posicionDondeSeCopia,sizeof(uint32_t));
	memcpy(&memoria+posicionACopiar,&datoACopiar,sizeof(uint32_t));
}

void liberarEspacioEnMemoria(t_primerNivel* unaTablaDe1erNivel){
	t_segundoNivel* tablaDe2doNivel = malloc(sizeof(t_segundoNivel));
	entradaTabla2doNivel* unaEntrada = malloc(sizeof(entradaTabla2doNivel));
	marco* marcoAsignado = malloc(sizeof(marco));

	for(int i = 0; i<entradasPorTabla;i++){
		tablaDe2doNivel = list_get(unaTablaDe1erNivel->tablasDeSegundoNivel,i);
		for(int j = 0;i<entradasPorTabla;j++){
			unaEntrada = list_get(tablaDe2doNivel->entradas,j);
			if(unaEntrada->presencia == 1 && unaEntrada->modificado ==0){
				sacarMarcoAPagina(unaEntrada);
				marcoAsignado = buscarMarco(unaEntrada->numeroMarco);
				marcoAsignado->marcoLibre=0;

			}
			if(unaEntrada->presencia == 1 && unaEntrada->modificado ==1){
				//escribirEnSwap(unaEntrada->numeroMarco,unaTablaDe1erNivel->pid);
				sacarMarcoAPagina(unaEntrada);
				marcoAsignado = buscarMarco(unaEntrada->numeroMarco);
				marcoAsignado->marcoLibre=0;
			}
		}

	}

list_clean(listaDePaginasEnMemoria);
}

char* nombreArchivoProceso(int pid){
	char* nombreArchivo = string_new();

	char* pidString = string_itoa(pid);
	nombreArchivo = pidString;

	string_append(&nombreArchivo,".swap");
	return nombreArchivo;
}

void escribirEnSwap(int numeroDeMarco,int pid,int desplazamiento,int numeroDePagina){//no funca bien

	char* nombreDelArchivo = nombreArchivoProceso(pid);
	FILE* archivoSwap = fopen(nombreDelArchivo, "a+");

	int posicionDeValorEnMemoria = numeroDeMarco*tamanioDePagina + sizeof(uint32_t)*desplazamiento;

	uint32_t* datoAEscribir = leerElPedido(posicionDeValorEnMemoria);

	log_info(logger,"leer: %u",*datoAEscribir);

//	for(int i=0; i<(tamanioDePagina/sizeof(uint32_t));i++){
//		int posicionDeValorEnMemoria = numeroDeMarco*tamanioDePagina + i*sizeof(uint32_t);
//
//		uint32_t* datoAEscribir = leerElPedido(posicionDeValorEnMemoria);
//
//		//char* datoAEscribirEnChar = string_itoa((uint32_t) datoAEscribir);
//		//string_append(&datoAEscribirEnChar,"\n");
//
//		//int posicionDeLaPaginaEnSwap = numeroDePagina* tamanioDePagina + i;
//
//		//fseek(archivoSwap, posicionDeLaPaginaEnSwap, SEEK_SET);
//
//		//fputs(datoAEscribirEnChar,archivoSwap);
//	}

	fclose(archivoSwap);
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


int algoritmoClock(t_list* listaDeEntradasDe2doNivel){
	entradaTabla2doNivel* unaEntrada = malloc(sizeof(entradaTabla2doNivel));

	for(int i = 0; i<list_size(listaDeEntradasDe2doNivel);i++){
		unaEntrada = list_get(listaDeEntradasDe2doNivel,i);

		if(unaEntrada->uso == 0){
				int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
				sacarMarcoAPagina(unaEntrada);
				return numeroDeMarcoAReemplazar;
		}
	}

	log_info(logger,"No hay ninguna pagina con bit de uso en 0");
	log_info(logger,"Por algoritmo reemplazo todos los bit de uso a 0 y busco de nuevo");
	reemplazarTodosLosUsoACero(listaDeEntradasDe2doNivel);

	for(int i = 0; i<list_size(listaDeEntradasDe2doNivel);i++){
		unaEntrada = list_get(listaDeEntradasDe2doNivel,i);

		if(unaEntrada->uso == 0){
				int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
				sacarMarcoAPagina(unaEntrada);
				return numeroDeMarcoAReemplazar;
		}
	}

}
// delegar funciones del if y hacer funcion de retorno

int algoritmoClockM (t_list* listaDeEntradasDe2doNivel){
	entradaTabla2doNivel* unaEntrada = malloc(sizeof(entradaTabla2doNivel));

	for(int i = 0; i<list_size(listaDeEntradasDe2doNivel);i++){
		unaEntrada = list_get(listaDeEntradasDe2doNivel,i);

		if(unaEntrada->uso == 0 && unaEntrada->modificado == 0){

			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			return numeroDeMarcoAReemplazar;

		}
	}

	log_info(logger,"No hay ninguna pagina con bit de uso y modificado en 0");
	log_info(logger,"Busco con bit de uso en 0 y modificado en 1");

	for(int i = 0; i<list_size(listaDeEntradasDe2doNivel);i++){
		unaEntrada = list_get(listaDeEntradasDe2doNivel,i);

		if(unaEntrada->uso == 0 && unaEntrada->modificado == 1){

			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			return numeroDeMarcoAReemplazar;
		}
	}

	log_info(logger,"No hay ninguna pagina con bit de uso en 0 y modificado en 1");
	log_info(logger,"Reemplazo todos los bit de uso en 0");
	reemplazarTodosLosUsoACero(listaDeEntradasDe2doNivel);
	log_info(logger,"Busco con bit de uso en 0 y modificado en 0 ");

	for(int i = 0; i<list_size(listaDeEntradasDe2doNivel);i++){
		unaEntrada = list_get(listaDeEntradasDe2doNivel,i);

		if(unaEntrada->uso == 0 && unaEntrada->modificado == 0){

			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
			return numeroDeMarcoAReemplazar;
		}
	}
	log_info(logger,"No hay ninguna pagina con bit de uso en 0 y modificado en 0");
	log_info(logger,"Busco con bit de uso en 0 y modificado en 1");

	for(int i = 0; i<list_size(listaDeEntradasDe2doNivel);i++){
		unaEntrada = list_get(listaDeEntradasDe2doNivel,i);

		if(unaEntrada->uso == 0 && unaEntrada->modificado == 1){

			int numeroDeMarcoAReemplazar = unaEntrada->numeroMarco;
			sacarMarcoAPagina(unaEntrada);
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

	}else{//Caso en el que ya el proceso tiene maxima cantidad de marcos por proceso y hay que desalojar 1
		if(strcmp(algoritmoDeReemplazo,"CLOCK") == 0){//PARA ESTOS CASOS NO ES LIST_ADD TENGO QUE REEMPLAZAR LA ENTRADA QUE SACO
			int marcoAAsignar = algoritmoClock(listaDePaginasEnMemoria);
			modificarPaginaACargar(unaEntrada,marcoAAsignar);
			int posicionAReemplazar = indiceDeEntradaAReemplazar(marcoAAsignar);
			list_replace(listaDePaginasEnMemoria, posicionAReemplazar, unaEntrada);


		}else if(strcmp(algoritmoDeReemplazo,"CLOCK M") == 0){
			int marcoAAsignar = algoritmoClockM(listaDePaginasEnMemoria);
			modificarPaginaACargar(unaEntrada,marcoAAsignar);
			int posicionAReemplazar = indiceDeEntradaAReemplazar(marcoAAsignar);
			list_replace(listaDePaginasEnMemoria, posicionAReemplazar, unaEntrada);

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
	unaEntrada->numeroMarco = -1;

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

		free(unaTablaDe1erNivel);
		return unaTablaDe2doNivel->numeroTabla; //El indice de entrada es igual al numero de tabla de 2do nivel
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
	}
		else{
			cargarPagina(unaEntradaTabla2doNivel);
			return unaEntradaTabla2doNivel->numeroMarco;
	//ver tema de si es una pagina con info para swap

		}
	}

}



void iterator(char* value) {
	log_info(logger,"%s", value);
}

// falta terminar programa


