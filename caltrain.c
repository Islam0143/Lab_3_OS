#include <pthread.h>
#include "caltrain.h"
#include <stdio.h>




void
station_init(struct station *station)
{
    station->passengersWaitingCount = 0;
    station->trainSeatsAvailable = 0;
    station->passengersOnBoard = 0;
    station->initialPassengersWaitingCount = 0;
    station->initialTrainSeatsAvailable = 0;
    pthread_mutex_init(&station->lock, NULL);
    pthread_cond_init(&station->trainArrived, NULL);
    pthread_cond_init(&station->trainReadyToLeave, NULL);
}

void
station_load_train(struct station *station, int count)
{
    if(count == 0 || station->passengersWaitingCount == 0) return;    //return immediately
    station->trainSeatsAvailable = count;               //no other functions edit these attributes so do not need to be in critical section
    station->initialTrainSeatsAvailable = count;                                //trainSeatsAvailable & passengersWaitingCount are edited by station_wait_for_train()
                                                                                // but after the train signals its arrival so do not have to be put in critical section
    station->initialPassengersWaitingCount = station->passengersWaitingCount;
    pthread_mutex_lock(&station->lock);     //acquire lock
    pthread_cond_broadcast(&station->trainArrived);
    while(station->trainSeatsAvailable != 0 && station->passengersWaitingCount != 0)
        pthread_cond_wait(&station->trainReadyToLeave,&station->lock);  //wait for passengers
    pthread_mutex_unlock(&station->lock);   //release lock
    station->passengersOnBoard = 0;     //clear it for the next train
}

void
station_wait_for_train(struct station *station)
{
    pthread_mutex_lock(&station->lock);     //acquire lock
    station->passengersWaitingCount++;      //a passenger came
    while(station->trainSeatsAvailable == 0)
        pthread_cond_wait(&station->trainArrived, &station->lock);  //wait for train arrival
    station->trainSeatsAvailable--;         //reserve one seat & decrement waiting passengers
    station->passengersWaitingCount--;
    pthread_mutex_unlock(&station->lock);   //release lock
}

void
station_on_board(struct station *station)
{
    station->passengersOnBoard++;   //no other functions edit this until the train leave is signaled which is done in this function
    pthread_mutex_lock(&station->lock);
    if(station->passengersOnBoard == station->initialTrainSeatsAvailable || station->passengersOnBoard == station->initialPassengersWaitingCount)   //if last passenger
        pthread_cond_signal(&station->trainReadyToLeave);   //signal the train to leave
    pthread_mutex_unlock(&station->lock);
}