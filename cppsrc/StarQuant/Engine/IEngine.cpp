/*****************************************************************************
 * Copyright [2019] 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/
#include <Engine/IEngine.h>
#include <Common/msgq.h>

namespace StarQuant {

IEngine::IEngine()
    : estate_(EState::DISCONNECTED) {
    init();
}

IEngine::~IEngine() {}

/**
 * @brief 初始化（默认空实现）
 * 子类可重写以加载配置、创建 messenger_ 等。
 */
void IEngine::init() {}

/**
 * @brief 启动引擎主循环（默认空实现）
 * 子类重写后通常在此处进入 connect -> 消息循环 -> disconnect 流程。
 */
void IEngine::start() {}

/**
 * @brief 停止引擎
 * 将 estate_ 置为 STOP，子类主循环通过检查 estate_ 退出。
 */
void IEngine::stop() {
    estate_ = EState::STOP;
}

}  // namespace StarQuant
