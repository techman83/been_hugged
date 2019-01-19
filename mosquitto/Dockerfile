FROM vimagick/mosquitto
RUN apk -v --update add \
	bash \
        python \
        py-pip \
        groff \
        less \
        mailcap && rm /var/cache/apk/* 
# Install aws-cli
RUN apk -Uuv add groff less python py-pip && rm /var/cache/apk/*
RUN pip install awscli && apk -v --purge del py-pip
COPY run.sh /bin/run.sh
ENTRYPOINT []
CMD ["/bin/run.sh"]
