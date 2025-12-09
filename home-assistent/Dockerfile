# ============================
# 1. Build da aplicação
# ============================
FROM eclipse-temurin:25-jdk AS build

WORKDIR /app

COPY mvnw .
COPY .mvn .mvn
COPY pom.xml .
RUN ./mvnw -q -e -DskipTests dependency:resolve

COPY . .

RUN ./mvnw -q -e -DskipTests package


# ============================
# 2. Runtime leve
# ============================
FROM eclipse-temurin:25-jre

WORKDIR /app

COPY --from=build /app/target/*.jar app.jar

EXPOSE 8080

ENTRYPOINT ["java", "-jar", "/app/app.jar"]
