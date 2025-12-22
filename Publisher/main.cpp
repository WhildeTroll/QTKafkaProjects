#include <QCoreApplication>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <librdkafka/rdkafka.h>

using namespace std;

void delivery_callback(rd_kafka_t* kafka,
                      const rd_kafka_message_t* msg,
                      void* opaque) {
    if (msg->err) {
        cerr << "Delivery failed: " << rd_kafka_err2str(msg->err) << endl;
    } else {
        string payload_str((char*)msg->payload, msg->len);
        cout << "Delivered JSON message to partition " << msg->partition
             << ", offset " << msg->offset << endl;
        cout << "Content: " << payload_str.substr(0, 100)
             << (payload_str.length() > 100 ? "..." : "") << endl;
    }
}

QByteArray create_sample_json(int sequence, int total) {
    QJsonObject j;
    j["id"] = QString("msg_%1").arg(rand() % 10000);
    j["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    j["sender"] = "QtProducer";
    j["sequence"] = sequence;
    j["total"] = total;

    QJsonObject data;
    data["type"] = "air";
    data["high"] = 45;
    j["data"] = data;

    QJsonObject metadata;
    metadata["version"] = "1.0";
    metadata["format"] = "json";
    metadata["compressed"] = false;
    j["metadata"] = metadata;

    QJsonDocument doc(j);
    return doc.toJson(QJsonDocument::Compact);
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    const char* brokers = "localhost:9092";
    const char* topic_str = "test_topic";

    // Создаем конфигурацию
    rd_kafka_conf_t* conf = rd_kafka_conf_new();
    char errstr[512];

    if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers,
                          errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        cerr << "Failed to set brokers: " << errstr << endl;
        return 1;
    }

    // Установка callback
    rd_kafka_conf_set_dr_msg_cb(conf, delivery_callback);

    // Создаем продюсера
    rd_kafka_t* producer = rd_kafka_new(RD_KAFKA_PRODUCER, conf,
                                        errstr, sizeof(errstr));
    if (!producer) {
        cerr << "Failed to create producer: " << errstr << endl;
        return 1;
    }

    // Создаем топик
    rd_kafka_topic_t* topic = rd_kafka_topic_new(producer, topic_str, NULL);
    if (!topic) {
        cerr << "Failed to create topic" << endl;
        rd_kafka_destroy(producer);
        return 1;
    }

    // Отправляем несколько JSON сообщений
    int messages_to_send = 5;

    for (int i = 0; i < messages_to_send; i++) {
        // Создаем JSON через Qt
        QByteArray json_data = create_sample_json(i + 1, messages_to_send);

        cout << "Sending JSON message " << (i + 1) << "/"
             << messages_to_send << ":" << endl;
        cout << json_data.constData() << endl << endl;

        // Отправляем
        if (rd_kafka_produce(
                topic,
                RD_KAFKA_PARTITION_UA,
                RD_KAFKA_MSG_F_COPY,
                (void*)json_data.constData(),
                json_data.size(),
                NULL, 0,
                NULL) == -1) {

            cerr << "Failed to produce message " << i
                 << ": " << rd_kafka_err2str(rd_kafka_last_error()) << endl;
        }

        // Небольшая задержка между сообщениями
        rd_kafka_poll(producer, 100);
    }

    // Ждем отправки всех сообщений
    while (rd_kafka_outq_len(producer) > 0) {
        rd_kafka_poll(producer, 100);
    }

    // Очистка
    rd_kafka_topic_destroy(topic);
    rd_kafka_destroy(producer);

    cout << "All JSON messages sent successfully!" << endl;

    return 0;
}
