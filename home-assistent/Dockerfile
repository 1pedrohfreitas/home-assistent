# ============================
# 1. Build
# ============================
FROM maven:3.9.8-eclipse-temurin-25 AS build

WORKDIR /app

COPY pom.xml .
RUN mvn -q -e -DskipTests dependency:resolve

COPY src ./src

RUN mvn -q -e -DskipTests package


# ============================
# 2. Runtime
# ============================
FROM eclipse-temurin:25-jre

WORKDIR /app

COPY --from=build /app/target/*.jar app.jar

EXPOSE 8080

ENTRYPOINT ["java", "-jar", "/app/app.jar"]
