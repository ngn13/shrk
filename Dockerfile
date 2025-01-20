######################
## build the server ##
######################
FROM golang:1.23.5 as go

COPY server /server
WORKDIR /server

RUN make CGO_ENABLED=0

#######################
## the actual runner ##
#######################
FROM alpine as main

RUN apk add dumb-init make

WORKDIR /shrk

COPY --from=go /server ./server
COPY scripts           ./scripts
COPY kernel            ./kernel
COPY user              ./user
COPY Makefile          ./

RUN make release
RUN chmod +x /shrk/scripts/init.sh

ENTRYPOINT ["dumb-init", "/shrk/scripts/init.sh"]
