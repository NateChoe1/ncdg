FROM debian:stable-slim AS build
RUN apt-get update -y && apt-get upgrade -y && apt-get install -y gcc make
COPY . /ncdg
WORKDIR /ncdg
RUN make

FROM debian:stable-slim AS run
COPY --from=build /ncdg/build/ncdg /usr/bin/ncdg

ENTRYPOINT [ "ncdg", "/dev/stdin" ]
