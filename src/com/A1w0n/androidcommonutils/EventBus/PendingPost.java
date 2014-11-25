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

import java.util.ArrayList;
import java.util.List;

/**
 * EventBus���¼������е�ĳ���¼���װ���¼����в�û�����κ�����װ���¼���������
 * ���ǿ��¼������е�ÿһ���¼���nextָ��(ָ������е���һ���¼�)���������е�
 *
 * ����¼���װ��ͷ����һ���¼��Ķ���أ����ڸ��ñ���Ķ���
 */
final class PendingPost {

    // �������Ķ���أ��������ñ���Ķ���
    private final static List<PendingPost> pendingPostPool = new ArrayList<PendingPost>();

    // �¼������߷����������¼�
    Object event;
    Subscription subscription;
    PendingPost next;

    /**
     * �ⲿû�а취����������캯��
     *
     * @param event
     * @param subscription
     */
    private PendingPost(Object event, Subscription subscription) {
        this.event = event;
        this.subscription = subscription;
    }

    /**
     * �Ӷ������ͷ��ȡһ������Ķ����ⲿ����û�취new����Ķ���ģ�ֻ��ͨ�������������ȡ
     * ����Ķ���
     *
     * @param subscription
     * @param event
     * @return
     */
    static PendingPost obtainPendingPost(Subscription subscription, Object event) {
        synchronized (pendingPostPool) {
            int size = pendingPostPool.size();
            if (size > 0) {
                // ���ض�������һ����������
                PendingPost pendingPost = pendingPostPool.remove(size - 1);
                pendingPost.event = event;
                pendingPost.subscription = subscription;
                pendingPost.next = null;
                return pendingPost;
            }
        }

        return new PendingPost(event, subscription);
    }

    /**
     * �����������˱���Ķ��󣬵�������ӿ����ѱ���Ķ��󻺴���������ͷ
     *
     * @param pendingPost
     */
    static void releasePendingPost(PendingPost pendingPost) {
        pendingPost.event = null;
        pendingPost.subscription = null;
        pendingPost.next = null;

        synchronized (pendingPostPool) {
            // Don't let the pool grow indefinitely
            // ��ֹ�������������
            if (pendingPostPool.size() < 10000) {
                pendingPostPool.add(pendingPost);
            }
        }
    }

}