# 1. Use an official Ubuntu Linux base image
FROM ubuntu:22.04

# 2. Prevent interactive prompts when installing packages
ENV DEBIAN_FRONTEND=noninteractive

# 3. Install the C++ compiler, Make, and the PostgreSQL C-library
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    libpq-dev \
    && rm -rf /var/lib/apt/lists/*

# 4. Set the working directory inside the container
WORKDIR /app

# 5. Copy all your C++ source files into the container
COPY . .

# 6. Compile the code for Linux
# libpq-dev headers live in /usr/include/postgresql on Ubuntu
RUN g++ -std=c++17 -Wall -I/usr/include/postgresql TcpServer.cpp PostgresManager.cpp main.cpp -lpq -o server

# 7. Expose the port your server listens on
EXPOSE 8080

# 8. Start the server when the container starts
CMD ["./server"]
