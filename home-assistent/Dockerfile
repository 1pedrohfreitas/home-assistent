# ============================
# 1. Build (JDK 24)
# ============================
FROM maven:3.9.11-eclipse-temurin-25-alpine AS build

WORKDIR /app

COPY pom.xml .
RUN mvn -q -e -DskipTests dependency:resolve

COPY src ./src

RUN mvn -q -e -DskipTests package


# ============================
# 2. Runtime (JRE 25)
# ============================
FROM eclipse-temurin:25-jre

WORKDIR /app

COPY --from=build /app/target/*.jar app.jar

EXPOSE 8080

ENTRYPOINT ["java", "-jar", "/app/app.jar"]
