
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

void *sending_thread() {

    int i;

    while(1)
    {

        PacketDescriptor *packet_descriptor;

        packet_descriptor = blockingReadBB(out_buffer);

        for(i = 0; i < 5; i++)
        {
            if(send_packet(network_device, packet_descriptor) == 1)
            {
                break;
            }
        }

        nonblocking_put_pd(fpds, packet_descriptor);
    }
    /*
        if(nonblockingReadBB(out_buffer, &packet_descriptor) == 0)
        {
            packet_descriptor = blockingReadBB(out_buffer);
        }

        for(i = 0; i < 4; i++)
        {
            if(send_packet(network_device, packet_descriptor) == 1)
            {
                break;
            }
            else
            {
                usleep(i * 10);
            }
        }
    }

    if(nonblocking_put_pd(fpds, &packet_descriptor) == 0)
    {
        blocking_put_pd(fpds, &packet_descriptor);
    }
    
    return NULL;*/
}


void *receiving_thread() {


    while(1)
    {
        PacketDescriptor *packet_descriptor;

        nonblocking_get_pd(fpds, &packet_descriptor);

        init_packet_descriptor(packet_descriptor);

        register_receiving_packetdescriptor(network_device, packet_descriptor);

        await_incoming_packet(network_device);
    }
    /*
        if(nonblocking_get_pd(fpds, &packet_descriptor) == 1)
        {
            init_packet_descriptor(&packet_descriptor);

            register_receiving_packetdescriptor(network_device, &packet_descriptor);

            await_incoming_packet(network_device);

            pid = packet_descriptor_get_pid(&packet_descriptor);

            while(1) 
            {
                if(nonblockingWriteBB(in_buffer[pid], &packet_descriptor) == 1)
                {
                    break;
                }
            }

            if(nonblocking_put_pd(fpds, &packet_descriptor) == 0)
            {
                blocking_put_pd(fpds, &packet_descriptor);
            }
        }
    }

    return NULL;i*/
}

void blocking_send_packet(PacketDescriptor *pd){
    blockingWriteBB(out_buffer, pd);
}

int  nonblocking_send_packet(PacketDescriptor *pd){
    return nonblockingWriteBB(out_buffer, pd);
}

void blocking_get_packet(PacketDescriptor **pd, PID pid){
    *pd = (PacketDescriptor*)blockingReadBB(in_buffer[pid]);
}

int  nonblocking_get_packet(PacketDescriptor **pd, PID pid){
    return nonblockingReadBB(in_buffer[pid], (void **)pd);
}

void init_network_driver(NetworkDevice *nd, void *mem_start, unsigned long mem_length,
                         FreePacketDescriptorStore **fpds_ptr){

   // pthread_t buffer_thread;
    //pthread_t send_thread;
    //pthread_t get_thread;

    network_device = nd;

    out_buffer = createBB(MAX_PID);

    int i;

    for(i = 0; i <= MAX_PID; i++)
    {
        in_buffer[i] = createBB(4);
    }

    *fpds_ptr = create_fpds();
    fpds = *fpds_ptr;

    create_free_packet_descriptors(fpds, mem_start, mem_length);

    //out_buffer = createBB(MAX_PID);

//    overflow_buffer = createBB(6);

  //  overflow_result = pthread_create(&buffer_thread, NULL, buffer_thread, NULL);
    int send_result, receive_result;
   
    send_result  = pthread_create(&send_thread, NULL, &sending_thread, NULL);
    receive_result = pthread_create(&receive_thread, NULL, &receiving_thread, NULL);

    if(send_result != 0)
    {
        DIAGNOSTICS("Error - unable to create send_thread");
    }

    if(receive_result != 0)
    {
        DIAGNOSTICS("Error - unable to create receive_thread");
    }
}


