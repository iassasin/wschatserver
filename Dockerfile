FROM sinair/ubuntu-gcc AS build
RUN apt install -y --no-install-recommends \
	libjsoncpp-dev \
	libssl-dev \
	libboost-dev \
	libboost-system-dev \
	libutfcpp-dev \
	libmysqlclient-dev \
	libmysqlcppconn-dev \
	libpq-dev \
	libpqxx-dev \
	&& apt clean
#RUN cd /tmp && wget https://github.com/open-source-parsers/jsoncpp/archive/refs/tags/1.9.4.tar.gz \
#	&& tar -zxf 1.9.4.tar.gz && cd jsoncpp-* \
#	&& mkdir build && cd build \
#	&& cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC_LIBS=ON .. \
#	&& make -j4 && make install \
#	&& ln -s . /usr/local/include/jsoncpp \
#	&& ln -s libjsoncpp_static.a /usr/local/lib/libjsoncpp.a
COPY . /app/
RUN cd /app && make clean all -j4

FROM sinair/ubuntu-gcc
RUN apt install -y --no-install-recommends \
	libjsoncpp25 \
	libssl3t64 \
	libmysqlclient21 \
	libmysqlcppconn7t64 \
	libboost-system1.83.0 \
	libpqxx-7.8t64 \
	&& apt clean
COPY --from=build /app/wsserver /app/
WORKDIR /app
CMD ["./wsserver"]