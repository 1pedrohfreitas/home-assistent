# ============================
# 1. Build (JDK 24)
# ============================
FROM eclipse-temurin:25-jdk-jammy AS build

WORKDIR /app

COPY pom.xml .
RUN mvn -q -e -DskipTests dependency:resolve

COPY src ./src

RUN mvn -q -e -DskipTests package


# ============================
# 2. Runtime (JRE 25)
# ============================
FROM eclipse-temurin:25-jre-jammy

WORKDIR /app

COPY --from=build /app/target/*.jar app.jar

EXPOSE 8080

ENTRYPOINT ["java", "-jar", "/app/app.jar"]
