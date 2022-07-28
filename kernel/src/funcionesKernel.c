#include "funcionesKernel.h"

//----------------------------- Para ser servidor de consola ------------------------------------
int iniciar_servidor(void)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo;

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

void recibir_mensaje(int socket_cliente) //No creo que haga falta
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

	int tamanioIdentificador;

	buffer = recibir_buffer(&size, socket_cliente);

	while(desplazamiento < size)
	{
		instruccion* unaInstruccion = malloc(sizeof(instruccion));
		memcpy(&tamanioIdentificador, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		unaInstruccion->identificador = malloc(tamanioIdentificador);
		memcpy(unaInstruccion->identificador, buffer+desplazamiento, tamanioIdentificador);
		desplazamiento+=tamanioIdentificador;
		memcpy(unaInstruccion->parametros, buffer+desplazamiento, sizeof(int[2]));
		desplazamiento+=sizeof(int[2]);
		list_add(listaDeInstrucciones, unaInstruccion);// Despues de esto habria que agragarlas al pcb
		//pcb->instrucciones = listaDeInstrucciones


	}
	free(buffer);
	return listaDeInstrucciones;
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
		instruccion* instruccion = malloc(sizeof(instruccion));
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
	desplazamiento+=sizeof(float);
	memcpy(&pcb->socket_cliente, buffer + desplazamiento, sizeof(int));

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

	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

int crear_conexion(char *ip, int puertoMemoria)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	char* puerto = string_itoa(puertoMemoria);  // convierte el entero a string para el getaddrinfo

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family,server_info->ai_socktype, server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

	freeaddrinfo(server_info);

	return socket_cliente;
}

void enviar_mensaje(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	if(socket_cliente == socketMemoria){
		paquete->codigo_operacion_memoria = MENSAJE_A_MEMORIA;
	}else if(socket_cliente == socketCpuDispatch){
		paquete->codigo_operacion_cpu = MENSAJE_A_MEMORIA;
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

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void agregar_a_paquete_kernel_cpu(t_pcb* pcb)
{
	tamanioTotalIdentificadores = 0;
	contadorInstrucciones = 0;
	desplazamiento = 0;
	paquete = crear_paquete(ENVIAR_PCB);
	list_iterate(pcb->instrucciones,(void*) obtenerTamanioIdentificadores);

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
//	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->socket_cliente), sizeof(int));
	//desplazamiento+=sizeof(int);
	paquete->buffer->size = desplazamiento;

	log_info(logger,"Le envio a cpu el pcb");
	enviar_paquete(paquete,socketCpuDispatch);
	eliminar_paquete(paquete);
	
	free(pcb);
}

void obtenerTamanioIdentificadores(instruccion* unaInstruccion) {
	tamanioTotalIdentificadores += strlen(unaInstruccion -> identificador)+1;
	contadorInstrucciones++;
}

void agregar_instrucciones_al_paquete(instruccion* instruccion) {
	char* id = instruccion->identificador;
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



