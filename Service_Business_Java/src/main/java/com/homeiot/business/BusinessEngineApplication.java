package com.homeiot.business;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.scheduling.annotation.EnableScheduling;

@SpringBootApplication
@EnableScheduling
public class BusinessEngineApplication {

    public static void main(String[] args) {
        SpringApplication.run(BusinessEngineApplication.class, args);
    }
}

