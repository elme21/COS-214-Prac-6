# TIVIDY web app: C++11 engine + HTTP server, built on Linux.
# Build:  docker build -t tividy .
# Run:    docker run --rm -p 8080:8080 tividy   then open http://localhost:8080

#  Stage 1: compile 
FROM ubuntu:24.04 AS build

RUN apt-get update \
 && apt-get install -y --no-install-recommends g++ \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Mirrors web/build.ps1, with two Linux changes:
#   -pthread          instead of -lws2_32 (Windows sockets are not needed on Linux)
#   -static-*         so the binary does not depend on this stage's libstdc++
RUN mkdir -p web/build \
 && g++ -std=c++11 -Wall -Wextra -include cstddef -I. -Iinclude -Iweb/third_party \
        -pthread -static-libstdc++ -static-libgcc \
        $(find src web/scenarios web/server -name '*.cpp') \
        -o web/build/tividy-web

#  Stage 2: small runtime image 
FROM ubuntu:24.04

WORKDIR /app
COPY --from=build /app/web/build/tividy-web web/build/tividy-web
COPY web/public web/public
COPY web/templates web/templates

EXPOSE 8080

# Same arguments as web/run.ps1 (check it if the server expects more).
CMD ["./web/build/tividy-web", "--public", "web/public"]
