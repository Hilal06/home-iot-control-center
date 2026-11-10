package com.homeiot.business.scheduler;

import com.homeiot.business.service.EnergyAnalyticsService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

@Component
public class LampAutomationScheduler {

    private static final Logger log = LoggerFactory.getLogger(LampAutomationScheduler.class);

    private final EnergyAnalyticsService energyAnalyticsService;

    public LampAutomationScheduler(EnergyAnalyticsService energyAnalyticsService) {
        this.energyAnalyticsService = energyAnalyticsService;
    }

    // Pukul 18:00 WIB: Menyalakan penerangan luar ruangan
    @Scheduled(cron = "0 0 18 * * *")
    public void scheduleEveningLighting() {
        log.info("AUTOMATION SCHEDULER [18:00]: Menyalakan lampu halaman depan & samping (yard_front, yard_side).");
    }

    // Pukul 05:30 WIB: Mematikan penerangan luar ruangan
    @Scheduled(cron = "0 30 5 * * *")
    public void scheduleMorningLighting() {
        log.info("AUTOMATION SCHEDULER [05:30]: Mematikan lampu halaman saat fajar (yard_front, yard_side).");
    }

    // Pukul 00:00 WIB tengah malam: Reset statistik akumulasi harian
    @Scheduled(cron = "0 0 0 * * *")
    public void scheduleDailyReset() {
        log.info("ENERGY SCHEDULER [00:00]: Melakukan rollover siklus hari baru untuk akuntansi energi.");
        energyAnalyticsService.resetDailyStats();
    }
}

