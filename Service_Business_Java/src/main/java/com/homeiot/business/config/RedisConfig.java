package com.homeiot.business.config;

import com.homeiot.business.consumer.TelemetryRedisConsumer;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.data.redis.connection.RedisConnectionFactory;
import org.springframework.data.redis.listener.ChannelTopic;
import org.springframework.data.redis.listener.RedisMessageListenerContainer;

@Configuration
public class RedisConfig {

    @Bean
    public RedisMessageListenerContainer redisContainer(
            RedisConnectionFactory connectionFactory,
            TelemetryRedisConsumer telemetryConsumer) {

        RedisMessageListenerContainer container = new RedisMessageListenerContainer();
        container.setConnectionFactory(connectionFactory);

        // Berlangganan ke channel telemetri stream dan status device
        container.addMessageListener(telemetryConsumer, new ChannelTopic("iot:telemetry:stream"));
        container.addMessageListener(telemetryConsumer, new ChannelTopic("iot:device:status"));

        return container;
    }
}

