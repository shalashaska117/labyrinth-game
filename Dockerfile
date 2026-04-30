FROM gcc:13

WORKDIR /app

COPY . .

RUN make clean || true
RUN make debug

CMD ["./server_app", "5000"]
