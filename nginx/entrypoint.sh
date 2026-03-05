#!/bin/sh
set -e

CONF_DIR="/etc/nginx/conf.d"
TEMPLATE_DIR="/etc/nginx/conf.d-templates"
CERT_FILE="/etc/letsencrypt/live/api.arkexperiment.xyz/fullchain.pem"

if [ -f "$CERT_FILE" ]; then
  cp "$TEMPLATE_DIR/https.conf" "$CONF_DIR/default.conf"
else
  cp "$TEMPLATE_DIR/http.conf" "$CONF_DIR/default.conf"
fi

exec nginx -g 'daemon off;'
