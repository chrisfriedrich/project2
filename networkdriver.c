/* Chris Friedrich - CIS 415 - Project 2 
 *
 * This is my own work. */

#include "packetdescriptor.h"
#include "destination.h"
#include "pid.h"
#include "diagnostics.h"
#include "packetdescriptorcreator.h"
#include "freepacketdescriptorstore__full.h"
#include "networkdevice.h"
#include "networkdriver.h"

#include "BoundedBuffer.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>


NetworkDevice *network_device;

FreePacketDescriptorStore *fpds;

BoundedBuffer *out_buffer;
BoundedBuffer *in_buffer[MAX_PID + 1];

pthread_t send_thread;
pthread_t receive_thread;


/* A helper method which works in conjunction with send_thread */
void *sending_thread() {

    while(1)
    {

        PacketDescriptor *packet_descriptor;

	/* Get the packet descriptor and wait for a received packet. */
        packet_descriptor = blockingReadBB(out_buffer);
        
        int i;
	
	/* Try five times and give up */
        for(i = 0; i < 5; i++)
        {
            if(send_packet(network_device, packet_descriptor) == 1)
            {
                /* Success! */
		break;
            }
        }

        nonblocking_put_pd(fpds, packet_descriptor);
    }
}

/* A helper method which works in conjunction with receive_thread */
void *receiving_thread() {

    while(1)
    {
        PacketDescriptor *packet_descriptor;
	
	/* Blocking required to proceed (have to have a packet) */
        nonblocking_get_pd(fpds, &packet_descriptor);
	/* Initialize the packet descriptor */
        init_packet_descriptor(packet_descriptor);
	/* Register with the Network Device */
        register_receiving_packetdescriptor(network_device, packet_descriptor);
	/* Wait for the next incoming packet */
        await_incoming_packet(network_device);
    }
}

/* Write packets to network device */
void blocking_send_packet(PacketDescriptor *pd){
    blockingWriteBB(out_buffer, pd);
}

/* Write packet to network device and return delivery status */
int  nonblocking_send_packet(PacketDescriptor *pd){
    return nonblockingWriteBB(out_buffer, pd);
}

/* Read packet from network */
void blocking_get_packet(PacketDescriptor **pd, PID pid){
    *pd = (PacketDescriptor*)blockingReadBB(in_buffer[pid]);
}

/* Read packet from network and return receipt status */
int  nonblocking_get_packet(PacketDescriptor **pd, PID pid){
    return nonblockingReadBB(in_buffer[pid], (void **)pd);
}


void init_network_driver(NetworkDevice *nd, void *mem_start, unsigned long mem_length,
                         FreePacketDescriptorStore **fpds_ptr){

    /* Initialize network device */
    network_device = nd;
	
    /* Create out bounded buffer as large as necessary */
    out_buffer = createBB(MAX_PID);
	
    int i;
    
    /* Create in bounded buffer */
    for(i = 0; i <= MAX_PID; i++)
    {
        in_buffer[i] = createBB(4);
    }

    /* Initialize free packet descriptor store */
    *fpds_ptr = create_fpds();
    fpds = *fpds_ptr;

    int fpds_result, send_result, receive_result;

    fpds_result = create_free_packet_descriptors(fpds, mem_start, mem_length);
    send_result  = pthread_create(&send_thread, NULL, &sending_thread, NULL);
    receive_result = pthread_create(&receive_thread, NULL, &receiving_thread, NULL);

    /* Return diagnostics */
    if(fpds_result < 1)
    {
    	DIAGNOSTICS("Error - unable to create fpds");
    }

    if(send_result != 0)
    {
        DIAGNOSTICS("Error - unable to create send_thread");
    }

    if(receive_result != 0)
    {
        DIAGNOSTICS("Error - unable to create receive_thread");
    }
}


