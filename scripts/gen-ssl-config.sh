#!/bin/bash

# TODO: use CA file

CERT_FILE="$( mktemp )"

openssl s_client -servername api.slack.com -connect api.slack.com:443 2>/dev/null > "${CERT_FILE}" < /dev/null

SLACK_API_SSL_CERT="$( openssl x509 -in "${CERT_FILE}" | \
	grep -v '^-----' | \
	awk '{ printf "  \"%s\\n\" \\\n", $0; }' )"
SLACK_API_SSL_FINGERPRINT="$( openssl x509 -in "${CERT_FILE}" -noout -fingerprint -sha1 | \
	awk -F'=' '{ gsub(":", " ", $2) ; print $2 }' )"

cat <<EOF
#define SLACK_API_SSL_CERT \\
  "-----BEGIN CERTIFICATE-----\\n" \\
${SLACK_API_SSL_CERT}
  "-----END CERTIFICATE-----\\n"
#define SLACK_API_SSL_CERT_FINGERPRINT "${SLACK_API_SSL_FINGERPRINT}"
EOF
