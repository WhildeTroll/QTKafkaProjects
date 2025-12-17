#include <QCoreApplication>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "librdkafka/rdkafka.h"

int main(int argc, char *argv[])
{
    char errstr[512];
    rd_kafka_t *rk;
    rd_kafka_conf_t *conf;
    rd_kafka_topic_conf_t *topic_conf;
    rd_kafka_topic_t *rkt;
    const char *hostname = "localhost:9092";
    const char *topic = "test-topic";
    const char *massage = "Hello Kafka";

    rd_kafka_conf_set(conf, "bootstrap.servers", hostname, errstr, sizeof (errstr) != RD_KAFKA_CONF_OK){
        fprintf(stderr, "Ошибка настройки брокеров %s\n", errstr);
        return 1;
    }

    topic_conf = rd_kafka_conf_new();

    rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof (errstr));
    if (!rk) {
        fprintf(stderr, "Ошибка создания продюсера %s\n", errstr);
        return 1;
    }

    rkt = rd_kafka_topic_new(rk, topic, topic_conf);
    if (!rkt){
        fprintf(stderr, "Ошибка создания топика %s\n", errstr);
        rd_kafka_destroy(rk);
    }
    rd_kafka_produce(rkt, RD_KAFKA_PARTITION_UA,
                  RD_KAFKA_MSG_F_COPY,
                  (void *)massage, strlen(massage),
                  NULL, 0,
                  NULL);


}
