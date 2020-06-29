//
// Created by assasin on 29.06.2020.
//

#ifndef WSSERVER_ROOMS_FWD_HPP
#define WSSERVER_ROOMS_FWD_HPP

#include <memory>

class Member;
class Room;

using MemberPtr = std::shared_ptr<Member>;
using RoomPtr = std::shared_ptr<Room>;

#endif //WSSERVER_ROOMS_FWD_HPP
