FROM sinair/ubuntu-gcc AS build
RUN apt install -y --no-install-recommends \
	libjsoncpp-dev \
	libssl-dev \
	libboost-dev \
	libboost-system-dev \
	libutfcpp-dev \
	libmysqlclient-dev \
	libmysqlcppconn-dev
COPY . /app/
RUN cd /app && make static -j4

FROM alpine:latest
COPY --from=build /app/wsserver /app/
WORKDIR /app
CMD ["./wsserver"]