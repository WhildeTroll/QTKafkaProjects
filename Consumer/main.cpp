#include <librdkafka/rdkafka.h>
#include <iostream>
#include <csignal>
#include <cstring>
#include <cinttypes>

static volatile sig_atomic_t run = 1;

static void stop(int sig) {
    run = 0;
}

static void msg_consume(rd_kafka_message_t *rkmessage, void *opaque) {
    if (rkmessage->err) {
        if (rkmessage->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) {
            printf("Достигнут конец раздела\n",
                   rkmessage->partition, rkmessage->offset);
        } else {
            fprintf(stderr, "Ошибка потребления: %s\n",
                    rd_kafka_message_errstr(rkmessage));
        }
        return;
    }


    std::cout << "Получено сообщение (раздел " << rkmessage->partition
                  << ", смещение " << rkmessage->offset << "): "
                  << std::string(static_cast<const char*>(rkmessage->payload), rkmessage->len)
                  << std::endl;
}

int main() {
    rd_kafka_t *consumer;
    rd_kafka_conf_t *conf;
    rd_kafka_resp_err_t err;
    char errstr[512];

    // Создание конфигурации потребителя
    conf = rd_kafka_conf_new();

    // Установка группы потребителей
    if (rd_kafka_conf_set(conf, "group.id", "test_consumer_group",
                          errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        fprintf(stderr, "Ошибка установки group.id: %s\n", errstr);
        return 1;
    }

    // Установка брокеров (по умолчанию localhost:9092)
    if (rd_kafka_conf_set(conf, "bootstrap.servers", "localhost:9092",
                          errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        fprintf(stderr, "Ошибка установки bootstrap.servers: %s\n", errstr);
        return 1;
    }

    // Установка автоматического подтверждения сообщений
    if (rd_kafka_conf_set(conf, "enable.auto.commit", "true",
                          errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        fprintf(stderr, "Ошибка установки enable.auto.commit: %s\n", errstr);
        return 1;
    }

    // Создание потребителя
    consumer = rd_kafka_new(RD_KAFKA_CONSUMER, conf, errstr, sizeof(errstr));
    if (!consumer) {
        fprintf(stderr, "Не удалось создать потребителя: %s\n", errstr);
        return 1;
    }

    // Подписка на топик
    rd_kafka_topic_partition_list_t *subscription =
        rd_kafka_topic_partition_list_new(1);
    rd_kafka_topic_partition_list_add(subscription, "test_topic", RD_KAFKA_PARTITION_UA);

    err = rd_kafka_subscribe(consumer, subscription);
    if (err) {
        fprintf(stderr, "Ошибка подписки: %s\n", rd_kafka_err2str(err));
        rd_kafka_topic_partition_list_destroy(subscription);
        rd_kafka_destroy(consumer);
        return 1;
    }

    rd_kafka_topic_partition_list_destroy(subscription);

    // Обработка сигналов для корректного завершения
    signal(SIGINT, stop);
    signal(SIGTERM, stop);

    printf("Ожидание сообщений из test_topic...\n");
    printf("Нажмите Ctrl+C для выхода\n");

    // Основной цикл потребления сообщений
    while (run) {
        rd_kafka_message_t *rkmessage;

        // Таймаут 1000 мс для получения сообщений
        rkmessage = rd_kafka_consumer_poll(consumer, 1000);

        if (rkmessage) {
            msg_consume(rkmessage, NULL);
            rd_kafka_message_destroy(rkmessage);
        }
    }

    // Закрытие потребителя
    printf("Закрытие потребителя...\n");
    rd_kafka_consumer_close(consumer);
    rd_kafka_destroy(consumer);

    printf("Потребитель завершил работу\n");
    return 0;
}
