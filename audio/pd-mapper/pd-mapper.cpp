/*
 * Copyright (c) 2018, Linaro Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <err.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <libqrtr.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <json/json.h>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
extern "C" {
#include "assoc.h"
#include "servreg_loc.h"
}

struct pd_map {
	const char *service;
	const char *domain;
	int instance;
};

static struct pd_map *pd_maps;

static void handle_get_domain_list(int sock, const struct qrtr_packet *pkt)
{
	struct servreg_loc_get_domain_list_resp resp = {};
	struct servreg_loc_get_domain_list_req req = {};
	struct servreg_loc_domain_list_entry *entry;
	unsigned char response_bytes[4096] = {};
    qrtr_packet resp_buf{};
    resp_buf.data = response_bytes;
    resp_buf.data_len = sizeof(response_bytes);
	const struct pd_map *pd_map = pd_maps;
	unsigned int txn = 0;
	ssize_t len;
	int ret;
    unsigned total = 0, matched = 0, offset = 0;
    size_t response_budget = 96; // conservative outer TLV overhead

	ret = qmi_decode_message(&req, &txn, pkt, QMI_REQUEST,
				 SERVREG_LOC_GET_DOMAIN_LIST,
				 servreg_loc_get_domain_list_req_ei);
	if (ret < 0) {
		resp.result.result = QMI_RESULT_FAILURE;
		resp.result.error = QMI_ERR_MALFORMED_MSG;
		goto respond;
	}

	req.name[sizeof(req.name)-1] = '\0';

	resp.result.result = QMI_RESULT_SUCCESS;
	resp.db_revision_valid = 1;
	resp.db_revision = 1;

    offset = req.offset_valid ? req.offset : 0;
    while (pd_map->service) {
        if (!strcmp(pd_map->service, req.name)) ++total;
        ++pd_map;
    }
    pd_map = pd_maps;
    while (pd_map->service) {
        if (!strcmp(pd_map->service, req.name) && matched++ >= offset) {
            const size_t cost = strlen(pd_map->domain) + 16;
            if (resp.domain_list_len >= sizeof(resp.domain_list) / sizeof(resp.domain_list[0]) ||
                response_budget + cost > sizeof(response_bytes)) break;
            entry = &resp.domain_list[resp.domain_list_len++];
            strcpy(entry->name, pd_map->domain);
            entry->name_len = strlen(pd_map->domain);
            entry->instance_id = pd_map->instance;
            response_budget += cost;
        }
        ++pd_map;
    }
    resp.domain_list_valid = resp.domain_list_len != 0;
    resp.total_domains_valid = 1;
    resp.total_domains = total;
    fprintf(stderr, "AERA PD mapper: %s: %u domains, page %u\n", req.name, total, resp.domain_list_len);

respond:
	len = qmi_encode_message(&resp_buf,
				 QMI_RESPONSE, SERVREG_LOC_GET_DOMAIN_LIST,
				 txn, &resp,
				 servreg_loc_get_domain_list_resp_ei);
	if (len < 0) {
		fprintf(stderr,
			"[PD-MAPPER] failed to encode get_domain_list response: %s\n",
			strerror(-len));
		return;
	}

	ret = qrtr_sendto(sock, pkt->node, pkt->port,
			  resp_buf.data, resp_buf.data_len);
	if (ret < 0) {
		fprintf(stderr,
			"[PD-MAPPER] failed to send get_domain_list response: %s\n",
			strerror(-ret));
	}
}

static int pd_load_map(const char *file, int pinned_fd = -1)
{
    static unsigned num_pd_maps;
    const int fd = pinned_fd >= 0 ? pinned_fd : open(file, O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return -1;
    struct stat metadata;
    if (fstat(fd, &metadata) || !S_ISREG(metadata.st_mode) ||
        metadata.st_size <= 0 || metadata.st_size > 131072) { close(fd); return -1; }
    std::string bytes(static_cast<size_t>(metadata.st_size), '\0');
    size_t done = 0;
    while (done < bytes.size()) {
        const ssize_t got = read(fd, bytes.data() + done, bytes.size() - done);
        if (got < 0 && errno == EINTR) continue;
        if (got <= 0) { close(fd); return -1; }
        done += static_cast<size_t>(got);
    }
    close(fd);
    // JsonCpp's Android build aborts on excessive recursion; reject it first.
    unsigned depth = 0;
    bool quoted = false, escaped = false;
    for (char c : bytes) {
        if (!c) return -1;
        if (quoted) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') quoted = false;
        } else if (c == '"') quoted = true;
        else if (c == '{' || c == '[') { if (++depth > 16) return -1; }
        else if (c == '}' || c == ']') { if (!depth) return -1; --depth; }
    }
    if (depth || quoted) return -1;
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    builder["failIfExtra"] = true;
    builder["rejectDupKeys"] = true;
    builder["stackLimit"] = 32;
    Json::Value root;
    std::string errors;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (!reader->parse(bytes.data(), bytes.data() + bytes.size(), &root, &errors)) return -1;
    if (!root.isObject()) return -1;
    const Json::Value& domain = root["sr_domain"];
    if (!domain.isObject() || !domain["soc"].isString() ||
        !domain["domain"].isString() || !domain["subdomain"].isString() ||
        !domain["qmi_instance_id"].isUInt() ||
        domain["qmi_instance_id"].asUInt() > INT_MAX) return -1;
    std::string domain_name = domain["soc"].asString() + "/" +
        domain["domain"].asString() + "/" + domain["subdomain"].asString();
    if (domain_name.size() >= sizeof(servreg_loc_domain_list_entry{}.name) ||
        domain_name.find('\0') != std::string::npos) return -1;
    const Json::Value& services = root["sr_service"];
    if (!services.isArray() || services.size() > 512 ||
        num_pd_maps + services.size() > 4096) return -1;
    std::vector<std::string> names;
    for (const auto& service : services) {
        if (!service.isObject() || !service["provider"].isString() ||
            !service["service"].isString()) return -1;
        std::string name = service["provider"].asString() + "/" + service["service"].asString();
        if (name.size() >= sizeof(servreg_loc_get_domain_list_req{}.name) ||
            name.find('\0') != std::string::npos) return -1;
        names.push_back(name);
    }
    auto* next = static_cast<pd_map*>(realloc(pd_maps,
        (num_pd_maps + names.size() + 1) * sizeof(pd_map)));
    if (!next) return -1;
    pd_maps = next;
    for (const auto& name : names) {
        char* service = strdup(name.c_str());
        char* path = strdup(domain_name.c_str());
        if (!service || !path) { free(service); free(path); return -1; }
        pd_maps[num_pd_maps++] = {service, path, static_cast<int>(domain["qmi_instance_id"].asUInt())};
    }
    pd_maps[num_pd_maps] = {};
    return 0;
}

static DIR* OpenScanDirectory(const char* path)
{
    const int fd = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return nullptr;
    DIR* directory = fdopendir(fd);
    if (!directory) close(fd);
    return directory;
}

/* Recovery uses private read-only mounts, not installed-ROM remoteproc paths. */
static int pd_enumerate_jsons(struct assoc *json_set)
{
    const char *paths[] = { "/mnt/vendor/audio_firmware/image",
        "/mnt/vendor/audio_firmware", "/mnt/vendor/audio_dsp",
        "/mnt/vendor/audio_dsp/adsp" };
    unsigned int index, files = 0;
    for (index = 0; index < sizeof(paths) / sizeof(paths[0]); ++index) {
        DIR *dir = OpenScanDirectory(paths[index]);
        struct dirent *entry;
        if (!dir) continue;
        while ((entry = readdir(dir))) {
            char file[PATH_MAX];
            size_t len = strlen(entry->d_name);
            if (len <= 4 || strcmp(entry->d_name + len - 4, ".jsn")) continue;
            if (snprintf(file, sizeof(file), "%s/%s", paths[index], entry->d_name) >= (int)sizeof(file)) continue;
            if (++files > 128) { closedir(dir); return -1; }
            const int fd = openat(dirfd(dir), entry->d_name, O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW);
            if (fd < 0) continue;
            assoc_set(json_set, file, reinterpret_cast<void*>(static_cast<intptr_t>(fd) + 1));
        }
        closedir(dir);
    }
    return 0;
}

static int pd_load_maps(void)
{
	struct assoc json_set;
	unsigned long it;
	const char *jsn;
    void* descriptor;
	int ret = 0;

	assoc_init(&json_set, 20);

	ret = pd_enumerate_jsons(&json_set);

	assoc_foreach(jsn, &descriptor, &json_set, it) {
        const int fd = static_cast<int>(reinterpret_cast<intptr_t>(descriptor) - 1);
        if (ret < 0) close(fd);
        else ret = pd_load_map(jsn, fd);
    }

	assoc_destroy(&json_set);

	return ret;
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    const int log_fd = open("/tmp/aera-pd-mapper.log", O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
    if (log_fd >= 0) {
        fchmod(log_fd, 0644);
        dup2(log_fd, STDERR_FILENO);
        close(log_fd);
    }
	struct sockaddr_qrtr sq;
	struct qrtr_packet pkt;
	unsigned int msg_id;
	socklen_t sl;
	char buf[4096];
	int ret;
	int fd;

	ret = pd_load_maps();
	if (ret)
		exit(1);

	if (!pd_maps) {
		fprintf(stderr, "no pd maps available\n");
		exit(1);
	}

	fd = qrtr_open(0);
	if (fd < 0) {
		fprintf(stderr, "failed to open qrtr socket\n");
		exit(1);
	}

	ret = qrtr_publish(fd, SERVREG_QMI_SERVICE,
			   SERVREG_QMI_VERSION, SERVREG_QMI_INSTANCE);
	if (ret < 0) {
		fprintf(stderr, "failed to publish service registry service\n");
		exit(1);
	}

    fprintf(stderr, "AERA PD mapper: service registry published\n");
	for (;;) {
		ret = qrtr_poll(fd, -1);
		if (ret < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				fprintf(stderr, "qrtr_poll failed\n");
				break;
			}
		}

		sl = sizeof(sq);
		ret = recvfrom(fd, buf, sizeof(buf), 0, reinterpret_cast<sockaddr*>(&sq), &sl);
		if (ret < 0) {
			ret = -errno;
			if (ret != -ENETRESET)
				fprintf(stderr, "[PD-MAPPER] recvfrom failed: %d\n", ret);
			return ret;
		}

		ret = qrtr_decode(&pkt, buf, ret, &sq);
		if (ret < 0) {
			fprintf(stderr, "[PD-MAPPER] unable to decode qrtr packet\n");
			return ret;
		}

		switch (pkt.type) {
		case QRTR_TYPE_DATA:
            if (pkt.data_len < 7) continue;
			ret = qmi_decode_header(&pkt, &msg_id);
			if (ret < 0)
				continue;

			switch (msg_id) {
			case SERVREG_LOC_GET_DOMAIN_LIST:
				handle_get_domain_list(fd, &pkt);
				break;
			case SERVREG_LOC_PFR:
				printf("[PD-MAPPER] pfr\n");
				break;
			};
			break;
		};
	}

	close(fd);

	return 0;
}
