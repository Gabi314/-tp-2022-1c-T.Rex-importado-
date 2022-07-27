#include "funcionesKernel.h"

//----------------------------- Para ser servidor de consola ------------------------------------
int iniciar_servidor(void)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(IP_KERNEL, PUERTO_KERNEL, &hints, &servinfo);

	// Creamos el socket de escucha del servidor
	socket_servidor = socket(servinfo->ai_family,servinfo->ai_socktype, servinfo->ai_protocol);

	// Asociamos el socket a un puerto
	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	// Escuchamos las conexiones entrantes
	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);
	log_trace(logger, "Listo para escuchar a consola");

	return socket_servidor;
}

t_config* iniciar_config(void){
	return config_create("kernel.config");
}

int esperar_cliente(int socket_servidor)
{
	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor, NULL, NULL); // acepto al cliente------------ ver esto----------------
	log_info(logger, "Se conecto la consola!");

	return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(int socket_cliente) //No creo que haga falta (estoy de acuerdo! atte: Gabi)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

t_list* recibir_paquete_int(int socket_cliente){
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);

	while(desplazamiento < size){
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		int valor = 0;
		memcpy(&valor, buffer+desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		list_add(valores, (void *)valor);
	}

	free(buffer);
	return valores;
}

t_list* recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;

	t_list* listaDeInstrucciones = list_create();
	t_list* identificadores = list_create();

	int tamanioIdentificador;

	buffer = recibir_buffer(&size, socket_cliente);

	while(desplazamiento < size)
	{
		instrucciones* instruccion = malloc(sizeof(instrucciones));
		memcpy(&tamanioIdentificador, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		instruccion->identificador = malloc(tamanioIdentificador);
		memcpy(instruccion->identificador, buffer+desplazamiento, tamanioIdentificador);
		desplazamiento+=tamanioIdentificador;
		memcpy(instruccion->parametros, buffer+desplazamiento, sizeof(int[2]));
		desplazamiento+=sizeof(int[2]);
		list_add(listaDeInstrucciones, instruccion);// Despues de esto habria que agragarlas al pcb
		//pcb->instrucciones = listaDeInstrucciones

		list_add(identificadores,instruccion->identificador);//Para que loguee los identificadores cuando les llegan de consola
	}
	free(buffer);
	return listaDeInstrucciones;
}

void enviarPID(int pid){//pasar entrada y nroTabla1ernivel
	t_paquete* paquete = crear_paquete(PAQUETE);
	agregar_a_paquete(paquete,&pid,sizeof(pid));
	log_info(logger,"Le envio a memoria mi pid que es %d",pid);
	enviar_paquete(paquete,socketMemoria);
	eliminar_paquete(paquete);
}

t_pcb* tomar_pcb(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;

	buffer = recibir_buffer(&size, socket_cliente);
	t_pcb* pcb = malloc(sizeof(t_pcb));
	pcb->instrucciones = list_create();
	int contadorInstrucciones;
	int i = 0;
		memcpy(&pcb->idProceso, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		memcpy(&pcb->tamanioProceso, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		memcpy(&contadorInstrucciones, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		while(i < contadorInstrucciones){
			instrucciones* instruccion = malloc(sizeof(instrucciones));
			int identificador_length;
			memcpy(&identificador_length, buffer + desplazamiento, sizeof(int));
			desplazamiento+=sizeof(int);
			instruccion->identificador = malloc(identificador_length);
			memcpy(instruccion->identificador, buffer+desplazamiento, identificador_length);
			desplazamiento+=identificador_length;
			memcpy(instruccion->parametros, buffer+desplazamiento, sizeof(int[2]));
			desplazamiento+=sizeof(int[2]);
			list_add(pcb -> instrucciones, instruccion);
			i++;
		}
		memcpy(&pcb->program_counter, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		memcpy(&pcb->tabla_paginas, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		memcpy(&pcb->estimacion_rafaga, buffer + desplazamiento, sizeof(float));

	free(buffer);
	return pcb;
}

//----------------------------- Para ser cliente de Cpu o de Memoria -------------------------------------------------

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	if(socketMemoria){
			memcpy(magic + desplazamiento, &(paquete->codigo_operacion_memoria), sizeof(int));
		}else if(socketCpuDispatch){
			memcpy(magic + desplazamiento, &(paquete->codigo_operacion_cpu), sizeof(int));
		}

	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

int crear_conexion(char *ip, int unPuerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	char puerto[20];
	sprintf(puerto,"%d",unPuerto); // convierte el entero a string para el getaddrinfo

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family,server_info->ai_socktype, server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

	freeaddrinfo(server_info);

	return socket_cliente;
}

void enviar_mensaje(char* mensaje, int socket_cliente, int cod_op)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	if(socket_cliente == socketMemoria){
			paquete->codigo_operacion_memoria = MENSAJE_LIBRERAR_ESTRUCTURAS;
		}else if(socket_cliente == socketCpuDispatch){
			paquete->codigo_operacion_cpu = MENSAJE_INTERRUPT;
		}

	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}


void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void agregar_instrucciones_al_paquete(instrucciones* instruccion) {
	void* id = instruccion->identificador;
	int longitudId = strlen(instruccion->identificador)+1;
	memcpy(paquete->buffer->stream + desplazamiento, &longitudId, sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, id, longitudId);
	desplazamiento+=longitudId;
	memcpy(paquete->buffer->stream + desplazamiento, &(instruccion->parametros), sizeof(int[2]));
	desplazamiento+=sizeof(int[2]);
	free(instruccion->identificador);
	free(instruccion);
}

t_paquete* crear_paquete(int cod_op)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	if(socketMemoria){
			paquete->codigo_operacion_memoria = cod_op;
		}else if(socketCpuDispatch){
			paquete->codigo_operacion_cpu = cod_op;
		}
	crear_buffer(paquete);
	return paquete;
}


void agregar_a_paquete_kernel_cpu(t_pcb* pcb, int op_code)
{
	tamanioTotalIdentificadores = 0;
	contadorInstrucciones = 0;
	desplazamiento = 0;
	paquete = crear_paquete(op_code);
	list_iterate(pcb->instrucciones, (void*) obtenerTamanioIdentificadores);
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanioTotalIdentificadores + contadorInstrucciones*sizeof(int[2]) + contadorInstrucciones*sizeof(int) + 6*sizeof(int) + sizeof(float));
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->idProceso), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->tamanioProceso), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &contadorInstrucciones, sizeof(int));
	desplazamiento+=sizeof(int);
	list_iterate(pcb->instrucciones, (void*) agregar_instrucciones_al_paquete);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->program_counter), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->tabla_paginas), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->estimacion_rafaga), sizeof(float));
	desplazamiento+=sizeof(float);

	paquete->buffer->size = desplazamiento;
	free(pcb);
}



void obtenerTamanioIdentificadores(instrucciones* instruccion) {
	tamanioTotalIdentificadores += (strlen(instruccion -> identificador)+1);
	contadorInstrucciones++;
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void eliminar_paquete_mensaje(t_paquete* paqueteMensaje)
{
	free(paqueteMensaje->buffer->stream);
	free(paqueteMensaje->buffer);
	free(paqueteMensaje);
}


void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}



void destruirProceso(t_pcb* proceso){
   // close(proceso->socket_cliente);
    free(proceso);
}


//-----------------PLANIFICADOR---------------




t_algoritmo_planificacion obtener_algoritmo(){

	 t_algoritmo_planificacion switcher;
	 char* algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");

	    //FIFO
	    if (strcmp(algoritmo,"FIFO") == 0)
	    {
	        switcher = FIFO;
	        log_info(logger, "El algoritmo de planificacion elegido es FIFO.");
	    }

	    //SFJ CON DESALOJO
	    if (strcmp(algoritmo,"SRT") == 0)
	    {
	        switcher = SRT;
	        log_info(logger, "El algoritmo de planificacion elegido es SJF.");
	    }
	    return switcher;
}// No se usa al final










  //--------------------TRANSICIONES---------------



t_pcb* sacarDeNew(){

	//sem_wait(&contadorNew);
	//pthread_mutex_lock(&mutexNew);

	t_pcb* proceso = queue_pop(colaNew);
	log_info(logger, "[NEW] Se saca el proceso de PID: %d de la cola", proceso->idProceso);

	//pthread_mutex_unlock(&mutexNew);

	return proceso;
}




void agregarABlocked(t_pcb* proceso){		//ver semaforos

	//sem_wait(&contadorExe);

	bool tienenMismoPID(void* elemento){

		if(proceso->idProceso == ((t_pcb *) elemento)->idProceso)
		    return true;
		else
			return false;
	}

//	list_remove_by_condition(colaExe, tienenMismoPID); NO HAY COLA DE EXE, SOLO SE EJECUTA UNO A LA VEZ

	//pthread_mutex_lock(&mutexBlock);

	list_add(colaBlocked, proceso);
	log_info(logger, "[BLOCK] Entra el proceso de PID: %d a la cola.", proceso->idProceso);

	//pthread_mutex_unlock(&mutexBlock);
	//sem_post(&multiprocesamiento);
	//sem_post(&contadorBlock);

	//sem_post(&analizarSuspension);
	//sem_wait(&suspensionFinalizada);
}

void sacarDeBlocked(t_pcb* proceso){

	//sem_wait(&contadorBlock);

	bool tienenMismoPID(void* elemento){

		if(proceso->idProceso == ((t_pcb *) elemento)->idProceso)
			return true;
		else
			return false;
		}

		//pthread_mutex_lock(&mutexBlock);

		list_remove_by_condition(colaBlocked, tienenMismoPID);
		log_info(logger, "[BLOCK] Sale el proceso de PID: %d de la cola.", proceso->idProceso);

		//pthread_mutex_unlock(&mutexBlock);
}




void agregarASuspendedBlocked(t_pcb* proceso){
/*
	//pthread_mutex_lock(&mutexBlockSuspended);

	proceso->suspendido = true;

	list_add(colaSuspendedBlocked, proceso);

	log_info(logger, "[SUSPENDEDBLOCKED] Ingresa el proceso de PID: %d a la cola.", proceso->idProceso);

	//pthread_mutex_unlock(&mutexBlockSuspended);

	size_t size = sizeof(sem_code)+sizeof(uint32_t);

	void* stream = malloc(size);

	sem_code opCode = SUSPEND;

	memcpy(stream, &opCode, sizeof(sem_code));
	memcpy(stream + sizeof(sem_code), &(proceso->idProceso), sizeof(uint32_t));

	send(proceso->socketMemoria, stream, size, 0);

	free(stream);
	 //Aca tenemos que mandar el proceso a disco*/
}

void sacarDeSuspendedBlocked(t_pcb* proceso){

	bool tienenMismoPID(void* elemento){

	if(proceso->idProceso == ((t_pcb *) elemento)->idProceso)
		return true;
	else
		return false;
	}

	//pthread_mutex_lock(&mutexBlockSuspended);

	list_remove_by_condition(colaSuspendedBlocked, tienenMismoPID);
	log_info(logger, "[SUSPENDEDBLOCKED] Sale el proceso de PID: %d de la cola.", proceso->idProceso);

	//pthread_mutex_unlock(&mutexBlockSuspended);
}

void agregarASuspendedReady(t_pcb* proceso){

	//pthread_mutex_lock(&mutexReadySuspended);

	queue_push(colaSuspendedReady, proceso);
	log_info(logger, "[SUSPENDEDREADY] Ingresa el proceso de PID: %d de la cola.", proceso->idProceso);

	//pthread_mutex_unlock(&mutexReadySuspended);

	//sem_post(&contadorReadySuspended);
	//sem_post(&medianoPlazo);
}

t_pcb* sacarDeSuspendedReady(){

	//sem_wait(&contadorReadySuspended);

	//pthread_mutex_lock(&mutexReadySuspended);

	t_pcb* proceso = queue_pop(colaSuspendedReady);
//	proceso->suspendido = false;
	log_info(logger, "[SUSPENDEDREADY] Sale el proceso de PID: %d de la cola.", proceso->idProceso);

	//pthread_mutex_unlock(&mutexReadySuspended);

	return proceso;
}















void newAReady(){

	while(1){

		//sem_wait(&largoPlazo);

		if(queue_size(colaSuspendedReady) != 0){

			//sem_post(&medianoPlazo);
		}else{

			t_pcb* proceso = sacarDeNew();

			proceso->estimacion_anterior = estimacionInicial;
			proceso->estimacion_rafaga = estimacionInicial;	//"estimacionInicial" va a ser una variable que vamos a obtener del cfg

			//sem_wait(&multiprogramacion);
			agregarAReady(proceso);
			//sem_post(&contadorProcesosEnMemoria);
		}
	} //aca hay que replanificar en caso de SRT?
}


void suspensionAReady(){

	while(1){

		//sem_wait(&medianoPlazo);

		if(queue_size(colaSuspendedReady) == 0){

			//sem_post(&largoPlazo);
		}else{

			t_pcb* proceso = sacarDeSuspendedReady();

			//sem_wait(&multiprogramacion);

			agregarAReady(proceso);

			//sem_post(&contadorProcesosEnMemoria);
		}
	}
}




int obtenerTiempoDeBloqueo(t_pcb* proceso){
	instrucciones* primeraInstruccion;
	primeraInstruccion = list_get(proceso->instrucciones,0);
	return primeraInstruccion->parametros[0];

}

bool supera_tiempo_maximo_bloqueado(t_pcb* proceso){
	if (obtenerTiempoDeBloqueo(proceso) > tiempoMaximoBloqueado)
		return true;
	else
		return false;

}







//------------------------HILOS-------------------------------


// * MULTIHILOS DE EJECUCION PARA ATENDER N CONSOLAS: esperan a recibir una consola y sus
// instrucciones para generar el pcb y asignar el proceso a NEW

void  recibir_consola(int servidor) {

		while(1) {
		pthread_t hilo1;

		int nuevo_cliente = esperar_cliente(servidor);
		int hiloCreado = pthread_create(&hilo1, NULL,&atender_consola,nuevo_cliente);

		pthread_detach(hilo1);

		}


	}



void atender_consola(int nuevo_cliente) {
			t_pcb* PCB;
			t_list* listaDeInstrucciones = recibir_paquete(nuevo_cliente); // supuestamente esto las devuelve

			t_list* proximaInstruccion = listaDeInstrucciones;

			PCB->idProceso = rand() % 985 + 16; // esto después va a ser un numero random por cada consola conectada
			PCB->tamanioProceso = sizeof(PCB);
			PCB->instrucciones = listaDeInstrucciones;
			PCB->program_counter = 1; // o 0?
			PCB->tabla_paginas = -1;
			PCB->estimacion_rafaga = estimacionInicial;
			PCB->estimacion_anterior = 0;
			PCB->rafagaMs = 0;
			PCB->horaDeIngresoAExe = 0;
			PCB->estado = NEW;


			t_pidXsocket*  datosCliente = malloc(sizeof(t_pidXsocket));
			datosCliente->pid = PCB->idProceso;
			datosCliente->socket = nuevo_cliente;

			pthread_mutex_lock(&consolaNueva);
			list_add(listaDeConsolas,datosCliente); // acá guardamos el socket
			pthread_mutex_unlock(&consolaNueva);

			pthread_mutex_lock(&encolandoPcb);
			agregarANew(PCB);
			pthread_mutex_unlock(&encolandoPcb);


			sem_post(&pcbEnNew);
		}


void agregarANew(t_pcb* proceso) {

	queue_push(colaNew, proceso);
	log_info(logger, "[NEW] Entra el proceso de PID: %d a la cola.", proceso->idProceso);
}

	/*
	ASIGNAR_MEMORIA(): si el grado de multiprogramacion lo permite, pasa el primer proceso de colaNew a READY, envia
	un mensaje al módulo Memoria para que inicialice sus estructuras necesarias y obtener el valor de la tabla de páginas
	del PCB. Además, si el algoritmo es SRT envía una interrupcion a cpu.
	 */
void asignar_memoria() {

		while(1){

		sem_wait(&pcbEnNew);
		sem_wait(&gradoDeMultiprogramacion);
		pthread_mutex_lock(&asignarMemoria); //falta dar prioridad a los de suspended and ready
		t_pcb* proceso = sacarDeNew();
		agregarAReady(proceso);
		pthread_mutex_unlock(&asignarMemoria);

	// para inicializar estructuras


		//enviarPID(proceso->idProceso);
		log_info(logger,"enviamos el pid %d", proceso->idProceso);
//		int cod_op = recibir_operacion(socketMemoria);
		int cod_op = recibir_operacion_prueba();
		log_info(logger,"recibimos el codigo de operacion de numero %d (deberia ser 1)", cod_op);

		t_list * paqueteNroTp = list_create;
		if(cod_op == PAQUETE_TP) {
	//	paqueteNroTp= recibir_paquete_int(socketMemoria);
		paqueteNroTp= recibir_paquete_int_prueba();
		proceso->tabla_paginas = list_get(paqueteNroTp,0);
		}

		log_info(logger,"recibimos el numero de tabla de paginas %d", proceso->tabla_paginas);

		if((algoritmoPlanificacion == SRT) && ejecucionActiva){
			log_info(logger,"desde asignar_memoria despertamos a atenderDesalojo");
			sem_post(&desalojarProceso);
			sem_wait(&procesoDesalojado);
			log_info(logger,"desde asignar_memoria sabemos que el proceso fue desalojado");
		}

		sem_post(&pcbEnReady);
		log_info(logger,"desde asignar_memoria despertamos a readyAExe");
		}

	}

void agregarAReady(t_pcb* proceso){
		list_add(colaReady, proceso);
		log_info(logger, "[READY] Entra el proceso de PID: %d a la cola.", proceso->idProceso);

}


void readyAExe(){

	while(1){


		sem_wait(&pcbEnReady);

		log_info(logger,".........se despierta readyAExe ");

		procesoAEjecutar = malloc(sizeof(t_pcb*));
		pthread_mutex_lock(&obtenerProceso);
		procesoAEjecutar = obtenerSiguienteDeReady(); //cambia la cola de ready, por eso el mutex
		pthread_mutex_unlock(&obtenerProceso);
		log_info(logger,"Desde readyAExe obtuvimos el proceso de pid: %d", procesoAEjecutar->idProceso);

		if(procesoAEjecutar != NULL) { // por las dudas dejo este if, pero se supone que los semaforos garantizan que no sea NULL
			log_info(logger,"Desde readyAExe mandamos al proceso de pid %d a ejecutar  ", procesoAEjecutar->idProceso);
			ejecutar(procesoAEjecutar);
			log_info(logger,"Desde readyAExe recibimos el proceso de pid: %d despues de ejecutar", procesoAEjecutar->idProceso);

			if(algoritmoPlanificacion == SRT){
				log_info(logger, "[EXEC] Ingresa el proceso de PID: %d con una rafaga de ejecucion estimada de %f milisegundos.", procesoAEjecutar->idProceso, procesoAEjecutar->estimacion_rafaga);
			}else{
				log_info(logger, "[EXEC] Ingresa el proceso de PID: %d", procesoAEjecutar->idProceso);
			}


		}else{
			log_info(logger, "[EXEC] No se logró encontrar un proceso para ejecutar");
		}

		if (list_size(colaReady)>0){
			sem_post(&pcbEnReady);
			log_info(logger,"desde readyAExe DESPERTAMOS DE NUEVO A readyAExe");
		}
	}
}

t_pcb* obtenerSiguienteFIFO(){

	t_pcb* procesoPlanificado = list_get(colaReady,0);
	return procesoPlanificado;
}

t_pcb* obtenerSiguienteSRT(){

	t_pcb* procesoPlanificado = NULL;
	t_pcb* procesoAux = NULL;
	int i;
	int indexARemover;
	float shortestJob;



	procesoAux = list_get(colaReady,0);


	indexARemover = 0;
	shortestJob = procesoAux->estimacion_rafaga;

	//itero por la lista de Ready

	log_info(logger,"PROCESOS EN READY: %d \n", list_size(colaReady));


	for(i=1;i<list_size(colaReady);i++){
	   	procesoAux = list_get(colaReady,i);
		if(shortestJob > procesoAux->estimacion_rafaga){
			shortestJob = procesoAux->estimacion_rafaga;
			indexARemover = i;
		}
	}
	procesoPlanificado = list_remove(colaReady, indexARemover);




	return procesoPlanificado;
}
t_pcb* obtenerSiguienteDeReady(){


	t_pcb * procesoPlanificado = malloc(sizeof(t_pcb*));

	// Aca dentro un SWITCH para los distintos algoritmos q llama a una funcion para cada uno
	switch(algoritmoPlanificacionActual){

		//CASO FIFO
		case FIFO:
			procesoPlanificado = obtenerSiguienteFIFO();
		break;

		//CASO SJF con desalojo
		case SRT:
			procesoPlanificado = obtenerSiguienteSRT();
		break;


	}

	// Devuelve NULL si no hay nada en ready
	// Caso contrario devuelve el que tiene mas prioridad segun el algoritmo que se este empleando
	return procesoPlanificado;
}



void ejecutar(t_pcb* proceso){


	time_t a = time(NULL);
	sem_wait(&cpuDisponible);
	log_info(logger,"Se duerme ejecutar()");
	pthread_mutex_lock(&ejecucion);
	proceso->horaDeIngresoAExe = ((float) a)*1000;
	proceso->estado = EXEC;
	pthread_mutex_lock(&ejecucion);

	ejecucionActiva = true;
	log_info(logger,"ejecucion activa es true!!");

	log_info(logger,"enviamos el proceso a cpu");
	simulador_de_cpu(1,proceso);

	//agregar_a_paquete_kernel_cpu(proceso,PCB_A_EJECUTAR);
	//enviar_paquete(paquete,socketCpuDispatch);
	//eliminar_paquete(paquete);

	//Aca mando el proceso a cpu y guardo el momento en el que arranca a ejecutar para el calculo de la estimacion
}




void atender_interrupcion_de_ejecucion(){

	while(1){
		int cod_op =  recibir_operacion(socketCpuDispatch);

		switch(cod_op) {// cuando reciba del cpu una interrupcion
		// obtener tiempo a partir de tomar el program_counter como indice
		case I_O:
			// PROXIMAMENTE
			break;
		case EXIT:
			pthread_mutex_lock(&mutexExit);
			procesoAFinalizar = tomar_pcb(socketCpuDispatch);
			pthread_mutex_unlock(&mutexExit);
			sem_post(&pcbExit);
			break;

		case INTERRUPT:
			pthread_mutex_lock(&mutexInterrupt);
			procesoADesalojar = tomar_pcb(socketCpuDispatch);
			pthread_mutex_unlock(&mutexInterrupt);
			sem_post(&pcbInterrupt);
			break;

		default:
			log_info(logger,"operacion invalida");
			break;
		}
		ejecucionActiva = false;
		sem_post(&cpuDisponible);
	}
}

void atenderDesalojo(){

	while(1){

	sem_wait(&desalojarProceso);

	log_info(logger,"se desperto atenderDesalojo");

	t_pcb * proceso = list_get(colaReady,0);

	simulador_de_cpu(2,proceso);
	log_info(logger,"se despierta al simuñador de cpu para continuar con el desalojo");
//	enviar_mensaje("Desalojar proceso",socketCpuInterrupt,MENSAJE_INTERRUPT);



	sem_wait(&pcbInterrupt);

	time_t a = time(NULL);
	float tiempoDeFin = ((float) a)*1000;

	pthread_mutex_lock(&desalojandoProceso);
	procesoADesalojar->rafagaMs = tiempoDeFin - procesoADesalojar->horaDeIngresoAExe;
	estimarRafaga(procesoADesalojar);
	agregarAReady(procesoADesalojar);
	pthread_mutex_unlock(&desalojandoProceso);
	log_info(logger,"Calculamos la rafaga del proceso a desalojar: PID[%d] - ESTIMACION-RAFAGA[%f]", procesoADesalojar->idProceso,procesoADesalojar->estimacion_rafaga);
	sem_post(&procesoDesalojado);
	log_info(logger,"volvemos a despertar asignar_memoria");
	}
}




void estimarRafaga(t_pcb* proceso){
	proceso->estimacion_anterior=proceso->estimacion_rafaga; // no estoy seguro de que la estimacion anterior sea necesaria
	proceso->estimacion_rafaga = alfa * proceso->rafagaMs + (1 - alfa) * proceso->estimacion_rafaga;

}

void terminarEjecucion(){

	while(1){
	sem_wait(&pcbExit);
	log_info(logger,"Se desperto terminarEjecucion...");
	t_pidXsocket * unaConsola = malloc(sizeof(t_pidXsocket));
	pthread_mutex_lock(&procesoExit);
	procesoAFinalizar->estado = TERMINATED;
	pthread_mutex_unlock(&procesoExit);
	log_info(logger,"Se cambia el estado del proceso %d a TERMINATED", procesoAFinalizar->idProceso);

//	enviar_mensaje("Liberar estructuras",socketMemoria,MENSAJE_LIBRERAR_ESTRUCTURAS);
	log_info(logger,"Se envia un mensaje a memoria para que libere estructuras");


	pthread_mutex_lock(&consolasExit);

	list_find(listaDeConsolas,unaConsola->pid == procesoAFinalizar->idProceso);

	log_info(logger, " Encontramos el socket del proceso %d, resulta que es %d", procesoAFinalizar->idProceso,unaConsola->socket);
//	enviar_mensaje("Finalizo la ejecucion",unaConsola->socket,MENSAJE_FINALIZAR_EXE); //TODAVIA FALTA HACER QUE CONSOLA PUEDA RECIBIR MENSAJES

	pthread_mutex_unlock(&consolasExit);
//	close(unaConsola->socket);

	log_info(logger, "Aumenta grado de multiprogramacion actual y termina terminarEjecucion!!");
	sem_post(&gradoDeMultiprogramacion);
	}
}

/*
				time_t a = time(NULL);									//de I/O se encarga de atenderla
				float tiempoDeFin = ((float) a)*1000;
				proceso->rafagaMs = tiempoDeFin - proceso->horaDeIngresoAExe;
				estimarRafaga(proceso);


				agregarABlocked(proceso);

				if(!supera_tiempo_maximo_bloqueado(proceso)){

					usleep(obtenerTiempoDeBloqueo(proceso)*1000);
					sacarDeBlocked(proceso);

					agregarAReady(proceso);

				}else{

					usleep(tiempoMaximoBloqueado*1000);

					sacarDeBlocked(proceso);

					agregarASuspendedBlocked(proceso);

					//Avisar a memoria

					usleep((obtenerTiempoDeBloqueo(proceso) - tiempoMaximoBloqueado)*1000);

					agregarASuspendedReady(proceso);

				}
*/






	/*
	  * FINALIZAR_PROCESO_Y_AVISAR_A_MEMORIA(): Recibe un PCB con motivo de finalizar el mismo, pasa al
		proceso al estado EXIT y da aviso al módulo Memoria para que éste libere sus estructuras. La idea sería tener
	  	un semaforo o algo que controle que la ejecucion del proceso sea la última
	 */

	/*
	 * FINALIZAR_PROCESO_Y_AVISAR_A_CONSOLA(): Una vez liberadas las estructuras de CPU, se dará aviso a la Consola
		de la finalización del proceso. La idea seria que espere el pcb de cpu con un mensaje que avise la liberación
		de las estructuras
	 */

	//* PLANIFICAR(): Llama constantemente al planificador.

/*
void planificar() {

		while(1) {
			readyAExe();
		}
	}
*/
	/*
	 * SUSPENDER(): si un proceso está bloqueado por un tiempo mayor al límite se llamará a una transición para
		           suspenderlo y se enviará un mensaje a memoria con la informacion necesaria.
	 */
/*
void suspender(){
		while(1){
		blockedASuspension(); // modificar: la idea sería que solo tome el primer elemento de la lista y
								//lo pase a bloqueado y suspendido
		}
	}
*/
	/*
	 * DESBLOQUEAR_SUSPENDIDO(): espera a que termine la entrada/salida de un proceso SUSPENDED-BLOCKED y llama
		a las transiciones necesarias para que pase a ser SUSPENDED-READY
	 */

void desbloquear_suspendido(){
		while(1){
		t_pcb * proceso = queue_pop(colaSuspendedBlocked);
		agregarASuspendedReady(proceso);
		}
	}


//-------------------------FUNCIONES DE PRUEBA------------------------------------

void inicializar_instrucciones_de_prueba() {
	/*
	instrucciones instruccionesDePrueba[20] = {
	{"EXIT",{-1,-1}},
	{"NO_OP",{5,-1}},{"NO_OP",{1,-1}},{"NO_OP",{2,-1}},
	{"I/O",{3000,-1}},{"I/O",{10000,-1}},{"I/O",{1000,-1}},{"I/O",{6000,-1}},
	{"WRITE",{4,42}},{"WRITE",{2,70}},{"WRITE",{1,20}},{"WRITE",{5,10}},{"WRITE",{12,102}},
	{"COPY",{3,4}},{"COPY",{5,2}},{"COPY",{8,1}},{"COPY",{5,15}},{"COPY",{30,12}},
	{"READ",{3,-1}},{"READ",{5,-1}}
	};
	*/
/*
	instrucciones * instruccion1 = malloc(sizeof(instrucciones));
	instrucciones * instruccion2 = malloc(sizeof(instrucciones));
	instrucciones * instruccion3 = malloc(sizeof(instrucciones));
	instrucciones * instruccion4 = malloc(sizeof(instrucciones));
	instrucciones * instruccion5 = malloc(sizeof(instrucciones));
*/

	instruccion1 = malloc(sizeof(instrucciones));
	instruccion2 = malloc(sizeof(instrucciones));
	instruccion3 = malloc(sizeof(instrucciones));
	instruccion4 = malloc(sizeof(instrucciones));
	instruccion5 = malloc(sizeof(instrucciones));
	instruccion6 = malloc(sizeof(instrucciones));
	instruccion7 = malloc(sizeof(instrucciones));
	instruccion8 = malloc(sizeof(instrucciones));
	instruccion9 = malloc(sizeof(instrucciones));
	instruccion10 = malloc(sizeof(instrucciones));
	instruccion11 = malloc(sizeof(instrucciones));
	instruccion12 = malloc(sizeof(instrucciones));
	instruccion13 = malloc(sizeof(instrucciones));
	instruccion14 = malloc(sizeof(instrucciones));
	instruccion15 = malloc(sizeof(instrucciones));
	instruccion16 = malloc(sizeof(instrucciones));
	instruccion17 = malloc(sizeof(instrucciones));
	instruccion18 = malloc(sizeof(instrucciones));
	instruccion19 = malloc(sizeof(instrucciones));
	instruccion20 = malloc(sizeof(instrucciones));


	instruccion1->identificador = "WRITE";
	instruccion1->parametros[0] = 4;
	instruccion1->parametros[1] = 42;

	instruccion2->identificador = "WRITE";
	instruccion2->parametros[0] = 2;
	instruccion2->parametros[1] = 70;

	instruccion3->identificador = "WRITE";
	instruccion3->parametros[0] = 1;
	instruccion3->parametros[1] = 20;

	instruccion4->identificador = "WRITE";
	instruccion4->parametros[0] = 5;
	instruccion4->parametros[1] = 10;

	instruccion5->identificador = "WRITE";
	instruccion5->parametros[0] = 12;
	instruccion5->parametros[1] = 102;

	instruccion6->identificador = "COPY";
	instruccion6->parametros[0] = 3;
	instruccion6->parametros[1] = 4;

	instruccion7->identificador = "COPY";
	instruccion7->parametros[0] = 5;
	instruccion7->parametros[1] = 2;

	instruccion8->identificador = "COPY";
	instruccion8->parametros[0] = 8;
	instruccion8->parametros[1] = 1;

	instruccion9->identificador = "COPY";
	instruccion9->parametros[0] = 5;
	instruccion9->parametros[1] = 15;

	instruccion10->identificador = "COPY";
	instruccion10->parametros[0] = 30;
	instruccion10->parametros[1] = 12;

	instruccion11->identificador = "READ";
	instruccion11->parametros[0] = 3;
	instruccion11->parametros[1] = -1;

	instruccion12->identificador = "READ";
	instruccion12->parametros[0] = 5;
	instruccion12->parametros[1] = -1;

	instruccion13->identificador = "I/O";
	instruccion13->parametros[0] = 3000;
	instruccion13->parametros[1] = -1;

	instruccion14->identificador = "I/O";
	instruccion14->parametros[0] = 10000;
	instruccion14->parametros[1] = -1;

	instruccion15->identificador = "I/O";
	instruccion15->parametros[0] = 1000;
	instruccion15->parametros[1] = -1;

	instruccion16->identificador = "I/O";
	instruccion16->parametros[0] = 6000;
	instruccion16->parametros[1] = -1;

	instruccion17->identificador = "NO_OP";
	instruccion17->parametros[0] = 5;
	instruccion17->parametros[1] = -1;

	instruccion18->identificador = "NO_OP";
	instruccion18->parametros[0] = 1;
	instruccion18->parametros[1] = -1;

	instruccion19->identificador = "NO_OP";
	instruccion19->parametros[0] = 2;
	instruccion19->parametros[1] = -1;

	instruccion20->identificador = "EXIT";
	instruccion20->parametros[0] = -1;
	instruccion20->parametros[1] = -1;

//

}

void inicializar_lista_de_prueba() {

//[1-5]: WRITE
//[6-10]: COPY
//[11-12]: READ
//[13-16]: I/O
//[17-19]: NO_OP
// 20: EXIT


lista3 = malloc(sizeof(t_list));
/*
lista50 = malloc(sizeof(t_list));
lista60 = malloc(sizeof(t_list));
lista270 = malloc(sizeof(t_list));
*/

list_add(lista3,instruccion18);
list_add(lista3,instruccion20);
/*
list_add(lista50,instruccion2);
list_add(lista50,instruccion15);
list_add(lista50,instruccion7);
list_add(lista50,instruccion5);
list_add(lista50,instruccion20);

list_add(lista60,instruccion12);
list_add(lista60,instruccion20);

list_add(lista270,instruccion4);
list_add(lista270,instruccion13);
list_add(lista270,instruccion10);
list_add(lista270,instruccion20);
*/




}

void atender_consola_prueba(int nuevo_cliente) {
				t_pcb* PCB = malloc(sizeof(t_pcb));
				inicializar_instrucciones_de_prueba();
				inicializar_lista_de_prueba();
				t_list * listaDeInstrucciones = malloc(sizeof(t_list));


				switch (nuevo_cliente){
					case 3:
						listaDeInstrucciones = lista3;
						break;
						/*
					case 50:
						listaDeInstrucciones = lista50;
						break;
					case 60:
						listaDeInstrucciones = lista60;
						break;
					case 270:
						listaDeInstrucciones = lista270;
						break;
*/
					default:
					log_info(logger,"Se acepto una consola invalida!");
					break;
					}


				log_info(logger,"Tenemos lista de instrucciones!");

				PCB->idProceso = rand() % 985 + 16;
				PCB->tamanioProceso = sizeof(PCB); // esto igual hay que calcularlo despues. Aunque no se cómo
				PCB->instrucciones = listaDeInstrucciones;
				PCB->program_counter = 1;
				PCB->tabla_paginas = -1;
				PCB->estimacion_rafaga = estimacionInicial;
				PCB->estimacion_anterior = 0;
				PCB->rafagaMs = 0;
				PCB->horaDeIngresoAExe = 0;
				PCB->estado = NEW;


				log_info(logger,"inicializamos el pcb");

				t_pidXsocket*  datosCliente = malloc(sizeof(t_pidXsocket));
				datosCliente->pid = PCB->idProceso;
				datosCliente->socket = nuevo_cliente;

				log_info(logger,"creamos y creamos la estructura datosCiente con id %d y socket %d", datosCliente->pid, datosCliente->socket);


				pthread_mutex_lock(&consolaNueva);
				list_add(listaDeConsolas,datosCliente); // acá guardamos el socket
				pthread_mutex_unlock(&consolaNueva);

				log_info(logger,"agregamos el pcb y el socket a la lista de consolas ");


				pthread_mutex_lock(&encolandoPcb);
				agregarANew(PCB);
				pthread_mutex_unlock(&encolandoPcb);



				log_info(logger,"agregamos el pcb a new");

				sem_post(&pcbEnNew);
			}

void recibir_consola_prueba(int servidor){
	int i = 1;

	while(1){
	pthread_t hilo1;
	log_info(logger,"esperamos una nueva consola");
	int nuevo_cliente = esperar_cliente_prueba(i); // es bloqueante. Si no llega el cliente, se bloquea el hilo.

		if (nuevo_cliente > 0) {

		int hiloCreado = pthread_create(&hilo1, NULL,&atender_consola_prueba,nuevo_cliente);
		pthread_detach(hilo1);

		}

		else {
			break; // la idea de esto seria que el hilo termine de ejecutarse pero no se si funque porque esta dentro de un else
		}

	i++;
	}

}

int esperar_cliente_prueba(int i){
	switch (i){
	case 1:
		usleep(300);
		log_info(logger, "Se conecto la consola 3!");
		return 3;

	/*
	case 2:
		usleep(4700);
		log_info(logger, "Se conecto la consola 50!");
		return 50;
	case 3:
		usleep(1000);
		log_info(logger, "Se conecto la consola 60!");
		return 60;
	case 4:
		usleep(21000);
		log_info(logger, "Se conecto la consola 270!");
		return 270;
		*/
	default:
		printf("No hay mas consolas");
	return -1;

	}


}

int recibir_operacion_prueba() {

	usleep(1000);
	return PAQUETE_TP;
}

t_list * recibir_paquete_int_prueba() {
	t_list * lista = list_create();
	list_add(lista,10);

	return lista;
}


void simulador_de_cpu(int operacion, t_pcb * proceso){

	switch(operacion) {
	case 1:
		log_info(logger,"cpu simulando la ejecucion de NO_OP por 1000 ms............ ");
		usleep(1000);
		log_info(logger,"Fin de la simulacion!");
		pthread_mutex_lock(&mutexExit);
		procesoAFinalizar = proceso;
		pthread_mutex_unlock(&mutexExit);
		log_info(logger,"Desde cpu nos llega el proceso a finalizar de pid %d ", procesoAFinalizar->idProceso);
		sem_post(&pcbExit);

		log_info(logger,"Desde el simulador despertamos terminarEjecucion");

	break;

	case 2:
		pthread_mutex_lock(&mutexInterrupt);
		procesoADesalojar = proceso;
		pthread_mutex_unlock(&mutexInterrupt);
		log_info(logger,"Desde cpu nos llega el proceso a desalojar de pid %d ", procesoADesalojar->idProceso);
		sem_post(&pcbInterrupt);
		log_info(logger,"Desde el simulador volvemos a despertar a atenderDesalojo");
	break;

	case 3:
	break;

	default:
		log_info(logger,"operacion no reconocida");
	break;

	}


	ejecucionActiva = false;
	log_info(logger,"ejecucionActiva ahora es false....");
	sem_post(&cpuDisponible);
	log_info(logger,"LA CPU VUELVE A ESTAR DISPONIBLE!");

}

