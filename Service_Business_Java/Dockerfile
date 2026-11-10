# ==========================================
# Minimal Production JRE Runtime
# ==========================================
FROM eclipse-temurin:17-jre-alpine AS runtime

WORKDIR /app

# Install curl for docker healthcheck
RUN apk add --no-cache curl

# Copy jar built via Gradle
COPY build/libs/*.jar app.jar

ENV SERVER_PORT=8081 \
    REDIS_HOST=redis \
    REDIS_PORT=6379 \
    JAVA_TOOL_OPTIONS="-XX:+UseG1GC -XX:MaxRAMPercentage=75.0"

EXPOSE 8081

HEALTHCHECK --interval=15s --timeout=3s --start-period=10s --retries=3 \
    CMD curl -f http://localhost:8081/health || exit 1

ENTRYPOINT ["java", "-jar", "app.jar"]
