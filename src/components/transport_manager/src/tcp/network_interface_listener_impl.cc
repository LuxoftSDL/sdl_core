#include "transport_manager/tcp/network_interface_listener_impl.h"
#include "platform_specific_network_interface_listener_impl.h"

namespace transport_manager {
namespace transport_adapter {

SDL_CREATE_LOGGERPTR( "TransportManager")

NetworkInterfaceListenerImpl::NetworkInterfaceListenerImpl(
    TcpClientListener* tcp_client_listener,
    const std::string designated_interface)
    : platform_specific_impl_(new PlatformSpecificNetworkInterfaceListener(
          tcp_client_listener, designated_interface)) {
  SDL_AUTO_TRACE();
}

NetworkInterfaceListenerImpl::~NetworkInterfaceListenerImpl() {
  SDL_AUTO_TRACE();
}

bool NetworkInterfaceListenerImpl::Init() {
  SDL_AUTO_TRACE();
  return platform_specific_impl_->Init();
}

void NetworkInterfaceListenerImpl::Deinit() {
  SDL_AUTO_TRACE();
  platform_specific_impl_->Deinit();
}

bool NetworkInterfaceListenerImpl::Start() {
  SDL_AUTO_TRACE();
  return platform_specific_impl_->Start();
}

bool NetworkInterfaceListenerImpl::Stop() {
  SDL_AUTO_TRACE();
  return platform_specific_impl_->Stop();
}

}  // namespace transport_adapter
}  // namespace transport_manager
