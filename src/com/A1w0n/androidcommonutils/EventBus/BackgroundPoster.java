/*
 * Copyright (C) 2012 Markus Junginger, greenrobot (http://greenrobot.de)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.A1w0n.androidcommonutils.EventBus;

import android.util.Log;

/**
 * Posts events in background.
 *
 * ����¼��������Ƿ�UI�̣߳����ֱ���ڷ����ߵ��̵߳��ö����ߵ��¼����պ���
 * ����¼���������UI�̣߳����ͨ����̨�߳��������¼������ߵ��¼����պ���
 *
 * ÿ��EventBus������һ��BackgroundPoster����
 * 
 * @author Markus
 */
final class BackgroundPoster implements Runnable {

    private final PendingPostQueue queue;
    private volatile boolean executorRunning;

    private final EventBus eventBus;

    BackgroundPoster(EventBus eventBus) {
        this.eventBus = eventBus;
        queue = new PendingPostQueue();
    }

    /**
     * ��������һ�����Σ������߳�A��B�����Ƕ�����EventBus.getDefault().post()�����պ����������¼����ֱ���һ��
     * onEventBackGround�����ߣ������߶���������Ե����������������A�Ƚ������������synchronize�����飬
     * ����EventBus.executorService.execute(this)����������executorRunning = true
     * ���߳�B�ͻ���ΪexecutorRunning == true�����޷�����EventBus.executorService.execute(this)
     *
     * ���������ô�죿
     *
     * ��ʵ�������û��ϵ����Ϊrun��������ͷ����¼�������ͷȡ�����е��¼����ҷַ���ȥ���������������ͬʱ���У�
     * ���е������
     *
     * @param subscription
     * @param event
     */
    public void enqueue(Subscription subscription, Object event) {
        PendingPost pendingPost = PendingPost.obtainPendingPost(subscription, event);

        synchronized (this) {
            queue.enqueue(pendingPost);
            if (!executorRunning) {
                executorRunning = true;
                EventBus.executorService.execute(this);
            }
        }
    }

    @Override
    public void run() {
        try {
            try {
                while (true) {
                    PendingPost pendingPost = queue.poll(1000);

                    if (pendingPost == null) {
                        synchronized (this) {
                            // Check again, this time in synchronized
                            pendingPost = queue.poll();
                            if (pendingPost == null) {
                                executorRunning = false;
                                // ���û���¼��ˣ����˳�whileѭ�������ҽ�������
                                return;
                            }
                        }
                    }

                    eventBus.invokeSubscriber(pendingPost);
                }
            } catch (InterruptedException e) {
                Log.w("Event", Thread.currentThread().getName() + " was interruppted", e);
            }
        } finally {
            executorRunning = false;
        }
    }

}
