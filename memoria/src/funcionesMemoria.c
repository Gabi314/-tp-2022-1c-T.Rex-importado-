#include "funcionesMemoria.h"

int iniciar_servidor(void)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	char* puertoMemoria = string_itoa(PUERTO_MEMORIA);// convierte el entero a string para el getaddrinfo

	getaddrinfo(IP_MEMORIA, puertoMemoria, &hints, &servinfo);

	// Creamos el socket de escucha del servidor
	socket_servidor = socket(servinfo->ai_family,servinfo->ai_socktype, servinfo->ai_protocol);

	// Asociamos el socket a un puerto
	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	// Escuchamos las conexiones entrantes
	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);
	log_trace(logger, "Listo para escuchar");

	return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor, NULL, NULL); // cuando se conecte cpu le envio tam_Pagina y cant_entradas_por_pagina con send()
	log_info(logger, "Se conecto un cliente!");


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

void recibir_mensaje(int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}



t_list* recibir_paquete_int(int socket_cliente)
{
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

//send(socket_cliente,&tamanioDePagina,sizeof(int),0); hay que hacer un enviar paquete
//------------------------------------------------------------------------------------
void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}


t_paquete* crear_paquete_tamanioDePagYCantEntradas(void)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = PAQUETE;
	crear_buffer(paquete);
	return paquete;
}

t_paquete* crear_paquete_nroTabla2doNivel(void)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = PAQUETE2;
	crear_buffer(paquete);
	return paquete;
}

t_paquete* crear_paquete_marco(void)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = PAQUETE3;
	crear_buffer(paquete);
	return paquete;
}

void enviarTamanioDePaginaYCantidadDeEntradas(int socket_cliente){
	t_paquete* paquete = crear_paquete_tamanioDePagYCantEntradas();
	log_info(logger,"Envio el tamanio de pag y cant entradas");

	agregar_a_paquete(paquete,&tamanioDePagina,sizeof(tamanioDePagina));
	agregar_a_paquete(paquete,&entradasPorTabla,sizeof(entradasPorTabla));


	enviar_paquete(paquete,socket_cliente);
	eliminar_paquete(paquete);
}

void enviarNroTabla2doNivel(int socket_cliente,int nroTabla2doNivel){
	t_paquete* paquete = crear_paquete_nroTabla2doNivel();
	log_info(logger,"Envio el numero de tabla de 2do nivel");

	agregar_a_paquete(paquete,&nroTabla2doNivel,sizeof(nroTabla2doNivel));


	enviar_paquete(paquete,socket_cliente);
	eliminar_paquete(paquete);
}

void enviarMarco(int socket_cliente, int marco){
	t_paquete* paquete = crear_paquete_marco();
	log_info(logger,"Envio el marco correspondiente el cual es %d",marco);

	agregar_a_paquete(paquete,&marco,sizeof(marco));

	enviar_paquete(paquete,socket_cliente);
	eliminar_paquete(paquete);
}


void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);// void* se utiliza para mandar espacion de usuario a memoria a escribir

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}
