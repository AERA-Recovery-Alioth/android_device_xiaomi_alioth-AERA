// SPDX-License-Identifier: Apache-2.0
#define main ProtectionDomainMapperMain
#include "pd-mapper/pd-mapper.cpp"
#undef main
#include <cassert>
static std::vector<unsigned char> reply;
extern "C" int qrtr_sendto(int, uint32_t, uint32_t, const void* data, unsigned size) {
    const auto* start = static_cast<const unsigned char*>(data);
    reply.assign(start, start + size); return size;
}
static void Query(unsigned offset, unsigned total) {
    servreg_loc_get_domain_list_req request{};
    strcpy(request.name, "avs/audio"); request.name_len = strlen(request.name);
    request.offset_valid = true; request.offset = offset;
    unsigned char request_bytes[512]{}; qrtr_packet packet{};
    packet.data = request_bytes; packet.data_len = sizeof(request_bytes);
    assert(qmi_encode_message(&packet, QMI_REQUEST, SERVREG_LOC_GET_DOMAIN_LIST, 7, &request, servreg_loc_get_domain_list_req_ei) > 0);
    handle_get_domain_list(0, &packet);
    assert(!reply.empty() && reply.size() <= 4096);
    qrtr_packet result{}; result.data = reply.data(); result.data_len = reply.size();
    servreg_loc_get_domain_list_resp decoded{}; unsigned txn = 0;
    assert(qmi_decode_message(&decoded, &txn, &result, QMI_RESPONSE, SERVREG_LOC_GET_DOMAIN_LIST, servreg_loc_get_domain_list_resp_ei) >= 0);
    assert(txn == 7 && decoded.result.result == QMI_RESULT_SUCCESS);
    assert(decoded.total_domains_valid && decoded.total_domains == total);
    if (offset < total) assert(decoded.domain_list_len > 0 && decoded.domain_list_len < total);
    else assert(decoded.domain_list_len == 0);
}
static int Load(const std::string& content) {
    char path[] = "/tmp/aera-pd-map-XXXXXX";
    int fd = mkstemp(path); assert(fd >= 0);
    assert(write(fd, content.data(), content.size()) == static_cast<ssize_t>(content.size()));
    close(fd); int result = pd_load_map(path); unlink(path); return result;
}
int main() {
    assert(Load(R"({"sr_domain":{"soc":"qcom","domain":"adsp","subdomain":"audio_pd","qmi_instance_id":74},"sr_service":[{"provider":"avs","service":"audio"}]})") == 0);
    assert(pd_maps && !strcmp(pd_maps[0].service, "avs/audio"));
    assert(!strcmp(pd_maps[0].domain, "qcom/adsp/audio_pd") && pd_maps[0].instance == 74);
    for (const char* bad : {"[]", "{}", "{", "{\"sr_domain\":null}",
         "{\"sr_domain\":{},\"sr_domain\":{}}", "{} trailing"}) assert(Load(bad) < 0);
    assert(Load(std::string(20, '[') + "0" + std::string(20, ']')) < 0);
    assert(Load(std::string(131073, ' ')) < 0);
    assert(Load(R"({"sr_domain":{"soc":"qcom","domain":"adsp","subdomain":"audio_pd","qmi_instance_id":-1},"sr_service":[]})") < 0);
    assert(Load(R"({"sr_domain":{"soc":"qcom","domain":"adsp","subdomain":"audio_pd","qmi_instance_id":4294967295},"sr_service":[]})") < 0);
    assert(Load(R"({"sr_domain":{"soc":"qcom","domain":"adsp","subdomain":"audio_pd","qmi_instance_id":1},"sr_service":[{}]})") < 0);
    std::string many = R"({"sr_domain":{"soc":"qcom","domain":"adsp","subdomain":")";
    many += std::string(210, 'a');
    many += R"(","qmi_instance_id":74},"sr_service":[)";
    for (int i = 0; i < 50; ++i) { if (i) many += ","; many += R"({"provider":"avs","service":"audio"})"; }
    many += "]}"; assert(Load(many) == 0);
    Query(0, 51); Query(20, 51); Query(51, 51);
    char link[] = "/tmp/aera-pd-link-XXXXXX"; int linkfd = mkstemp(link); assert(linkfd >= 0); close(linkfd); unlink(link);
    assert(symlink("/etc/passwd", link) == 0); assert(pd_load_map(link) < 0); unlink(link);
    assert(mkfifo(link, 0600) == 0); assert(pd_load_map(link) < 0); unlink(link);
    char directory[] = "/tmp/aera-pd-dir-XXXXXX"; assert(mkdtemp(directory));
    std::string directory_link = std::string(directory) + "-link";
    assert(symlink(directory, directory_link.c_str()) == 0);
    assert(OpenScanDirectory(directory_link.c_str()) == nullptr);
    DIR* safe_directory = OpenScanDirectory(directory); assert(safe_directory); closedir(safe_directory);
    unlink(directory_link.c_str()); rmdir(directory);
    for (pd_map* map = pd_maps; map->service; ++map) { free(const_cast<char*>(map->service)); free(const_cast<char*>(map->domain)); }
    free(pd_maps); pd_maps = nullptr;
    puts("PD mapper: valid audio map, malformed/duplicate keys, nesting, file size, service schema and instance bounds passed");
}
