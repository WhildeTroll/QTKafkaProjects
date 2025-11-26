#include <QCoreApplication>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "librdkafka/rdkafka.h"

int main(int argc, char *argv[])
{
    char hostname = 'localhost:9092';
    char errstr[512];
    rd_kafka_conf_t *conf = rd_kafka_conf_new();
    rd_kafka_produce();
}
