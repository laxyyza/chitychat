# k8s-examples
A collection of example Kubernetes YAML files.  
>[!NOTE]
>These are for reference only and not intended for production use.

These examples assume:
- Your cluster has an Ingress controller installed.
- Persistent storage is available (e.g., Longhorn).
- A private, insecure container registry is accessible at `192.168.69.1:5000`.
- The following hostnames are mapped to your Ingress IP in your local `/etc/hosts` (or equivalent):
  - `chitychat.local`
  - `cc-sfs.local`

Create TLS secret:
```bash
kubectl create secret tls chitychat-tls --cert=CERT_PATH --key=KEY_PATH
```

Create Env secret:
```bash
kubectl create secret generic chitychat-secrets --from-env-file=.env-k8s
```
