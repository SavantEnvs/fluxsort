FROM debian:bookworm as builder

RUN apt update && \
    DEBIAN_FRONTEND=noninteractive apt install build-essential clang -y

ADD . /fluxsort
WORKDIR /fluxsort/fuzz

RUN make

FROM debian:bookworm
COPY --from=builder /fluxsort/fuzz/fluxsort-fuzz /